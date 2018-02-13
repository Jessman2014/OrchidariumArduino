#include <SHT1X.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_TSL2561_U.h>
#include <TimeLib.h>
#include <DS3232RTC.h>
#include <ArduinoJson.h>

#define TIME_HEADER  "T"   // Header tag for serial time sync message
#define TIME_REQUEST  7    // ASCII bell character requests a time sync message 

//variables for storing values
float tempC = 0;
float tempF = 0;
float humidity = 0;
float light = 0;
float oldTempF = 0;
float oldHumidity = 0;
float oldLight = 0;
bool valuesChanged = false;
bool justSetTime = false;

int relayPwr = A0;

int coolMist = 2; //1
int resevoirRefill = 3; //2
int warmMist = 5; //3
int heater = 4; //4
int fogger = 7; //5
int mister = 6; //6
int twoDayWater = 9; //7
int mainLED = 8; //8

int ledFan = 10;

//Timers
unsigned long warmMistTimer = 0; 
unsigned long twoDayWaterTimer = 0;

//Create an instance of the SHT1X sensor
SHT1x sht15(12, 13);//Data, SCK
//Adafruit_TSL2561_Unified tsl = Adafruit_TSL2561_Unified(TSL2561_ADDR_FLOAT, 12345);
StaticJsonBuffer<200> jsonBuffer;
JsonObject& root = jsonBuffer.createObject();

void setup()
{
	Serial.begin(9600); // Open serial connection to report values to host
	pinMode(relayPwr, OUTPUT);
	digitalWrite(relayPwr, HIGH);
  setSyncProvider(RTC.get);
  if(timeStatus()!= timeSet) 
     Serial.println("Unable to sync with the RTC");
  //else
    // Serial.println("RTC has set the system time");  

     
	//Serial.println("Light Sensor Test"); Serial.println("");
  //TODO remove above serial println

	// Initialise the light sensor
	//tsl.begin();
  //Serial.println("after ts1 begin");
  //TODO remove above serial println
	/*if (!tsl.begin())
	{
		// There was a problem detecting the TSL2561 ... check your connections
		Serial.print("Ooops, no TSL2561 detected ... Check your wiring or I2C ADDR!");
		while (1);
	}
	*/

	/* Setup the sensor gain and integration time */
	//configureSensor();
  //Serial.println("after configuration of sensors");
  //TODO remove above serial println
	configureRelaysAndFans();
}

/**************************************************************************/
/*
Configures the gain and integration time for the TSL2561
*/
/**************************************************************************/
void configureSensor()
{
	/* You can also manually set the gain or enable auto-gain support */
	// tsl.setGain(TSL2561_GAIN_1X);      /* No gain ... use in bright light to avoid sensor saturation */
	// tsl.setGain(TSL2561_GAIN_16X);     /* 16x gain ... use in low light to boost sensitivity */
	//USE THIS tsl.enableAutoRange(true);            /* Auto-gain ... switches automatically between 1x and 16x */

										  /* Changing the integration time gives you better sensor resolution (402ms = 16-bit data) */
	//USE THIS tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_13MS);
	/* fast but low resolution */
	// tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_101MS);  /* medium resolution and speed   */
	// tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_402MS);  /* 16-bit data but slowest conversions */

	/* Update these values depending on what you've set above! */
	// Serial.println("------------------------------------");
	// Serial.print  ("Gain:         "); Serial.println("Auto");
	// Serial.print  ("Timing:       "); Serial.println("13 ms");
	// Serial.println("------------------------------------");
}


void configureRelaysAndFans()
{
	pinMode(coolMist, OUTPUT);
	pinMode(resevoirRefill, OUTPUT);
	pinMode(warmMist, OUTPUT);
	pinMode(heater, OUTPUT);
	pinMode(fogger, OUTPUT);
  pinMode(mister, OUTPUT);
  pinMode(twoDayWater, OUTPUT);
	pinMode(mainLED, OUTPUT);
  pinMode(ledFan, OUTPUT);

	//make sure relays are off at start
	digitalWrite(coolMist, HIGH);
 digitalWrite(resevoirRefill, HIGH);
 digitalWrite(warmMist, HIGH);
 digitalWrite(heater, HIGH);
 digitalWrite(fogger, HIGH);
 digitalWrite(mister, HIGH);
 digitalWrite(twoDayWater, HIGH);
 digitalWrite(mainLED, HIGH);
  digitalWrite(ledFan, LOW);
}

//-------------------------------------------------------------------------------------------
void loop()
{
	readSensor();
	findTime();
	checkConditionsForRelays();
  if (valuesChanged && timeStatus() == timeSet){
    root.printTo(Serial);
    Serial.println();
  }
	delay(5000);
}
//-------------------------------------------------------------------------------------------
void readSensor()
{
  valuesChanged = false;
	// Read values from the sensor
	tempC = sht15.readTemperatureC();
	tempF = sht15.readTemperatureF();
	humidity = sht15.readHumidity();


	/* Get a new sensor event */
	//sensors_event_t event;
	//tsl.getEvent(&event);

	/* Display the results (light is measured in lux) */
	//light = event.light;
	//if (event.light)
	//{
	//	Serial.print(event.light); Serial.println(" lux");
	//}
	//else
	//{
	//	/* If event.light = 0 lux the sensor is probably saturated
	//	and no reliable data could be generated! */
	//	Serial.println("Sensor overload");
	//}

  if (abs(oldTempF - tempF) > 2)
  {
    valuesChanged = true;
    oldTempF = tempF;
  }

  if (abs(oldHumidity - humidity) > 3)
  {    
    valuesChanged = true;
    oldHumidity = humidity;
  }

    root["TemperatureF"] = tempF;
    root["Humidity"] = humidity;
    root["Lux"] = 0;
}

void findTime()
{
	if (timeStatus() == timeNotSet)
 {
		Serial.println("Unable to sync with the RTC");
    setTime(1357041600); // Jan 1 2013
    delay(5000);
 }
  else
  {
    root["dateRecorded"] = (unsigned long)now();
  }
}

void digitalClockDisplay()
{
	// digital clock display of the time
	Serial.print(hour());
	printDigits(minute());
	printDigits(second());
	Serial.print(" ");
	Serial.print(day());
	Serial.print(" ");
	Serial.print(month());
	Serial.print(" ");
	Serial.print(year());
	Serial.println();
}

void printDigits(int digits) {
	// utility function for digital clock display: prints preceding colon and leading 0
	Serial.print(":");
	if (digits < 10)
		Serial.print('0');
	Serial.print(digits);
}

void checkConditionsForRelays()
{
	// cool mist humidifier
	if (shouldCoolMistHumidifierBeOn() && !isRelayOn(coolMist)) {
    turnRelayOn(coolMist, "CoolMist");
	}
	else if (!shouldCoolMistHumidifierBeOn() && isRelayOn(coolMist)) {
    turnRelayOff(coolMist, "CoolMist");
	}
		
	// heater
	if (shouldHeaterBeOn() && !isRelayOn(heater))
	{
		turnRelayOn(heater, "Heater");
	}
	else if (!shouldHeaterBeOn() && isRelayOn(heater))
	{
		turnRelayOff(heater, "Heater");
	}

	// warm mist
	if (shouldWarmMistHumidifierBeOn() && !isRelayOn(warmMist))
	{
		turnRelayOn(warmMist, "WarmMist");
		warmMistTimer = (unsigned long)now();
	}
	else if (!shouldWarmMistHumidifierBeOn() && isRelayOn(warmMist))
	{
		turnRelayOff(warmMist, "WarmMist");
	}
 
  // fogger and mister
  if (isFoggingTime() && !isRelayOn(fogger))
  {
    turnRelayOn(fogger, "Fogger");
    turnRelayOn(mister, "Mister");
  }
  else if (!isFoggingTime() && isRelayOn(fogger))
  {
    turnRelayOff(fogger, "Fogger");
    turnRelayOff(mister, "Mister");
  }

  // two day watering
  if (isTwoDayWateringTime() && !isRelayOn(twoDayWater))
  {
    turnRelayOn(twoDayWater, "TwoDayWater");
  }
  else if (!isTwoDayWateringTime() && isRelayOn(twoDayWater))
  {
    turnRelayOff(twoDayWater, "TwoDayWater");
  }

  // LEDs
  if (isDayTime() && !isRelayOn(mainLED))
  {
    turnRelayOn(mainLED, "MainLED");
    digitalWrite(ledFan, HIGH);
    deviceCommandPrint("TurnOn", "SecondaryLED");
  }
  else if (!isDayTime() && isRelayOn(mainLED))
  {
    turnRelayOff(mainLED, "MainLED");
    digitalWrite(ledFan, LOW);
    deviceCommandPrint("TurnOff", "SecondaryLED");
  }
}



//*****************  HELPER FUNCTIONS  *******************

bool shouldCoolMistHumidifierBeOn()
{
		if (isRelayOn(coolMist))
		{
			return humidity < 80;
		}
		else
		{
			return humidity < 70;
		}
}

bool shouldWarmMistHumidifierBeOn()
{
  return (shouldCoolMistHumidifierBeOn() && shouldHeaterBeOn()) || 
  (isRelayOn(warmMist) && (unsigned long)now() - warmMistTimer < 900);
}

bool shouldHeaterBeOn()
{
	if (isRelayOn(heater))
	{
		return !isWarmEnough();
	}
	else
	{
		return isCold();
	}
}

bool isDayTime()
{
	return hour() > 8 && hour() < 18;
}

bool isBonsaiWateringTime()
{
	return weekday() == 3 && hour() == 20 && minute() == 0;
}

bool isTwoDayWateringTime()
{
  return (weekday() % 2) == 0 && hour() == 12 && minute() == 0;
}

bool isFoggingTime()
{
  return hour() == 12 && minute() > 0 && minute() < 10;
}

//bool isDry()
//{
//	return humidity < 70;
//}
//
//bool isHumidEnough()
//{
//	return humidity > 80;
//}

bool isWarmEnough()
{
	if (isDayTime()) {
		return tempF > 77;
	}
	else {
		return tempF > 67;
	}
}

bool isCold()
{
	if (isDayTime()) {
		return tempF < 73;
	}
	else {
		return tempF < 63;
	}
}

void turnRelayOn(int pinNum, String relayName)
{
	digitalWrite(pinNum, LOW);
  deviceCommandPrint("TurnOn", relayName);
}

void turnRelayOff(int pinNum, String relayName)
{
	digitalWrite(pinNum, HIGH);
  deviceCommandPrint("TurnOff", relayName);
}

void deviceCommandPrint(String action, String device)
{
  if (timeStatus() == timeSet)
    Serial.println("{\"Message\":\"" + action + device + "\"}");
}

bool isRelayOn(int pinNum)
{
	return digitalRead(pinNum) == LOW;
}
