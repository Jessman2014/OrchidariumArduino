/******************************************************************************
SHT15 Example

Connections:
GND  -> A2
Vcc  -> A3
DATA -> A4
SCK  -> A5

******************************************************************************/
#include <SHT1X.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_TSL2561_U.h>
#include <TimeLib.h>
#include <ArduinoJson.h>

#define TIME_HEADER  "T"   // Header tag for serial time sync message
#define TIME_REQUEST  7    // ASCII bell character requests a time sync message 

//variables for storing values
float tempC = 0;
float tempF = 0;
float humidity = 0;
float light = 0;
int relayPwr = A0;
int mainLED = 5;
int coolMist = 3;
int coolMistFan = 9;
int warmMist = 6;
int heatLamp = 7;
int bonsai = 8;
int ledFan = 10;

//Create an instance of the SHT1X sensor
SHT1x sht15(12, 13);//Data, SCK
Adafruit_TSL2561_Unified tsl = Adafruit_TSL2561_Unified(TSL2561_ADDR_FLOAT, 12345);
StaticJsonBuffer<200> jsonBuffer;
JsonObject& root = jsonBuffer.createObject();

void setup()
{
	Serial.begin(9600); // Open serial connection to report values to host
	pinMode(relayPwr, OUTPUT);
	digitalWrite(relayPwr, HIGH);

	//Serial.println("Light Sensor Test"); Serial.println("");

	// Initialise the light sensor
	tsl.begin();
	/*if (!tsl.begin())
	{
		// There was a problem detecting the TSL2561 ... check your connections 
		Serial.print("Ooops, no TSL2561 detected ... Check your wiring or I2C ADDR!");
		while (1);
	}
	*/

	/* Setup the sensor gain and integration time */
	configureSensor();
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
	tsl.enableAutoRange(true);            /* Auto-gain ... switches automatically between 1x and 16x */

										  /* Changing the integration time gives you better sensor resolution (402ms = 16-bit data) */
	tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_13MS);
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
	//pinMode(mainLED, OUTPUT);
	pinMode(coolMist, OUTPUT);
	pinMode(coolMistFan, OUTPUT);
	pinMode(warmMist, OUTPUT);
	pinMode(heatLamp, OUTPUT);
	pinMode(bonsai, OUTPUT);
	pinMode(ledFan, OUTPUT);

	//make sure relays are off at start
	turnRelayOff(mainLED);
	turnRelayOff(coolMist);
	turnRelayOff(warmMist);
	turnRelayOff(heatLamp);
	turnRelayOff(bonsai);
}

//-------------------------------------------------------------------------------------------
void loop()
{
	readSensor();
	findTime();
	checkConditionsForRelays();
	root.printTo(Serial);
	Serial.println();
	delay(5000);
}
//-------------------------------------------------------------------------------------------
void readSensor()
{
	// Read values from the sensor
	tempC = sht15.readTemperatureC();
	tempF = sht15.readTemperatureF();
	humidity = sht15.readHumidity();
	root["TemperatureF"] = tempF;
	root["Humidity"] = humidity;
	/*Serial.print(" Temp = ");
	Serial.print(tempF);
	Serial.print("F, ");
	Serial.print(tempC);
	Serial.println("C");
	Serial.print(" Humidity = ");
	Serial.print(humidity);
	Serial.println("%");*/

	/* Get a new sensor event */
	sensors_event_t event;
	tsl.getEvent(&event);

	/* Display the results (light is measured in lux) */
		light = event.light;
		root["Lux"] = light;
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
}

void findTime()
{
	if (Serial.available())
	{
		processSyncMessage();
	}
	if (timeStatus() == timeNotSet)
		Serial.println("waiting for sync message");
	else
	{
		//digitalClockDisplay();
		unsigned long seconds = (unsigned long)now();
		root["dateRecorded"] = seconds;
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

void processSyncMessage()
{
	unsigned long pctime;
	const unsigned long DEFAULT_TIME = 1357041600; // Jan 1 2013

	if (Serial.find(TIME_HEADER)) {
		pctime = Serial.parseInt();
		if (pctime >= DEFAULT_TIME) { // check the integer is a valid time (greater than Jan 1 2013)
			setTime(pctime); // Sync Arduino clock to the time received on the serial port
		}
	}
}

time_t requestSync()
{
	Serial.write(TIME_REQUEST);
	return 0; // the time will be sent later in response to serial mesg
}

void checkConditionsForRelays()
{
	// cool mist humidifier
	if (shouldCoolMistHumidifierBeOn())
		turnCoolMistOn();
	else
		turnCoolMistOff();

	// heat lamp
	if (shouldHeatLampBeOn())
	{
		turnRelayOn(heatLamp);
		root["HeatLampOn"] = true;
	}
	else
	{
		turnRelayOff(heatLamp);
		root["HeatLampOn"] = false;
	}

	// warm mist
	if (isRelayOn(coolMist) && isRelayOn(heatLamp))
	{
		turnRelayOn(warmMist);
		root["BoilerOn"] = true;
	}
	else
	{
		turnRelayOff(warmMist);
		root["BoilerOn"] = false;
	}

	// LEDs
	if (isDayTime())
	{
		//Serial.println("it's day time");
		turnRelayOn(mainLED);
		digitalWrite(ledFan, HIGH);
		root["MainLED"] = true;
		root["SecondaryLEDs"] = true;
	}
	else
	{
		//Serial.println("it's night time");
		turnRelayOff(mainLED);
		digitalWrite(ledFan, LOW);
		root["MainLED"] = false;
		root["SecondaryLEDs"] = false;
	}

	// bonsai pump
	if (isBonsaiWateringTime())
	{
		turnRelayOn(bonsai);
		root["BonsaiOn"] = true;
	}
	else
	{
		turnRelayOff(bonsai);
		root["BonsaiOn"] = false;
	}
}



//*****************  HELPER FUNCTIONS  *******************

bool shouldCoolMistHumidifierBeOn()
{
	if (isDayTime())
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
	else
	{
		if (isRelayOn(coolMist))
		{
			return humidity < 60;
		}
		else
		{
			return humidity < 50;
		}
	}
}

bool shouldHeatLampBeOn()
{
	if (isDayTime())
	{
		if (isRelayOn(heatLamp))
		{
			return !isWarmEnough();
		}
		else
		{
			return isCold();
		}
	}
	else
	{
		return false;
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
	return tempF > 77;
}

bool isCold()
{
	return tempF < 73;
}

void turnRelayOn(int pinNum)
{
	digitalWrite(pinNum, LOW);
}

void turnRelayOff(int pinNum)
{
	digitalWrite(pinNum, HIGH);
}

bool isRelayOn(int pinNum)
{
	return digitalRead(pinNum) == LOW;
}

void turnCoolMistOn()
{
	turnRelayOn(coolMist);
	digitalWrite(coolMistFan, HIGH);
	root["FoggerOn"] = true;
}

void turnCoolMistOff()
{
	turnRelayOff(coolMist);
	digitalWrite(coolMistFan, LOW);
	root["FoggerOn"] = false;
}