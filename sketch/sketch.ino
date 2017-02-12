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
#include <Time.h>

// time sync to PC is HEADER followed by Unix time_t as ten ASCII digits
#define TIME_MSG_LEN 11 
// Header tag for serial time sync message
#define TIME_HEADER 'T' 
// ASCII bell character requests a time sync message 
#define TIME_REQUEST 7 

//variables for storing values
float tempC = 0;
float tempF = 0;
float humidity = 0;
int relayPwr = A0;
int mainLED = 3;
int coolMist = 5;
int warmMist = 6;
int heatLamp = 7;
int bonsai = 8;

//Create an instance of the SHT1X sensor
SHT1x sht15(12, 13);//Data, SCK
Adafruit_TSL2561_Unified tsl = Adafruit_TSL2561_Unified(TSL2561_ADDR_FLOAT, 12345);
//delacre output pins for powering the sensor

void setup()
{
  Serial.begin(9600); // Open serial connection to report values to host
  pinMode(relayPwr, OUTPUT);
  digitalWrite(relayPwr, HIGH);

  Serial.println("Light Sensor Test"); Serial.println("");
  
  /* Initialise the sensor */
  if(!tsl.begin())
  {
    /* There was a problem detecting the TSL2561 ... check your connections */
    Serial.print("Ooops, no TSL2561 detected ... Check your wiring or I2C ADDR!");
    while(1);
  }
  
  /* Setup the sensor gain and integration time */
  configureSensor();
  configureRelays();
}
//-------------------------------------------------------------------------------------------
void loop()
{
  readSensor();
  findTime();

  delay(1000);
}
//-------------------------------------------------------------------------------------------
void readSensor()
{
  // Read values from the sensor
  tempC = sht15.readTemperatureC();
  tempF = sht15.readTemperatureF();
  humidity = sht15.readHumidity();  
  Serial.print(" Temp = ");
  Serial.print(tempF);
  Serial.print("F, ");
  Serial.print(tempC);
  Serial.println("C");
  Serial.print(" Humidity = ");
  Serial.print(humidity); 
  Serial.println("%");

  /* Get a new sensor event */ 
  sensors_event_t event;
  tsl.getEvent(&event);
 
  /* Display the results (light is measured in lux) */
  if (event.light)
  {
    Serial.print(event.light); Serial.println(" lux");
  }
  else
  {
    /* If event.light = 0 lux the sensor is probably saturated
       and no reliable data could be generated! */
    Serial.println("Sensor overload");
  }
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
  tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_13MS);      /* fast but low resolution */
  // tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_101MS);  /* medium resolution and speed   */
  // tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_402MS);  /* 16-bit data but slowest conversions */

  /* Update these values depending on what you've set above! */  
  // Serial.println("------------------------------------");
  // Serial.print  ("Gain:         "); Serial.println("Auto");
  // Serial.print  ("Timing:       "); Serial.println("13 ms");
  // Serial.println("------------------------------------");
}


void configureRelays()
{
  pinMode(mainLED, OUTPUT);
  pinMode(coolMist, OUTPUT);
  pinMode(warmMist, OUTPUT);
  pinMode(heatLamp, OUTPUT);
  pinMode(bonsai, OUTPUT);
}

void findTime()
{
  if (Serial.available() )
  {
    processSyncMessage();
  }
  if (timeStatus() == timeNotSet)
    Serial.println("waiting for sync message");
  else
    digitalClockDisplay();
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
 // if time sync available from serial port, update time and return true
  while (Serial.available() >= TIME_MSG_LEN ) 
  {
    // time message consists of header & 10 ASCII digits
    char c = Serial.read() ;
    Serial.print(c);
    if ( c == TIME_HEADER ) 
    {
      time_t pctime = 0;
      for (int i = 0; i < TIME_MSG_LEN - 1; i++) 
      {
        c = Serial.read();
        if ( c >= '0' && c <= '9') 
        {
          pctime = (10 * pctime) + (c - '0') ; // convert digits to a number
        }
      }
      setTime(pctime); 
      // Sync Arduino clock to the time received on the serial port
    }
  }
}

bool IsDayTime()
{

}