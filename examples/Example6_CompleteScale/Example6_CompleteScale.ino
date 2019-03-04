/*
  Use the Qwiic Scale to read load cells and scales
  By: Nathan Seidle @ SparkFun Electronics
  Date: March 3rd, 2019
  License: This code is public domain but you buy me a beer if you use this
  and we meet someday (Beerware license).

  This example shows how to setup a scale complete with tear, zero offset,
  and linear calibration. For even more information, checkout the
  firmware that runs on the OpenScale product: https://github.com/sparkfun/OpenScale/blob/master/firmware/OpenScale/OpenScale.ino

  SparkFun labored with love to create this code. Feel like supporting open
  source? Buy a board from SparkFun!
  https://www.sparkfun.com/products/15242

  Hardware Connections:
  Plug a Qwiic cable into the Qwiic Scale and a RedBoard Qwiic
  If you don't have a platform with a Qwiic connection use the SparkFun Qwiic Breadboard Jumper (https://www.sparkfun.com/products/14425)
  Open the serial monitor at 9600 baud to see the output
*/

#include <Wire.h>
#include <EEPROM.h> //Needed to record user settings

#include "SparkFun_Qwiic_Scale_NAU7802_Arduino_Library.h" // Click here to get the library: http://librarymanager/All#SparkFun_NAU8702

NAU7802 myScale; //Create instance of the NAU7802 class

long setting_calibration_factor; //Value used to convert the load cell reading to lbs or kg
long setting_tare_point; //Zero value that is found when scale is tared

long zeroOffset; //This is the reading when there is nothing on the scale
long knownOffset; //This is the reading when there is a known weight on the scale

long currentReading; //This is the lastest reading from the sensor

#define LOCATION_CALIBRATION_FACTOR 0
#define LOCATION_TARE_POINT 10

void setup()
{
  Serial.begin(9600);
  Serial.println("Qwiic Scale Example");

  Wire.begin();
  Wire.setClock(400000);

  if (myScale.begin() == false)
  {
    Serial.println("Scale not detected. Please check wiring. Freezing...");
    while (1);
  }
  Serial.println("Scale detected!");
}

void loop()
{
  if (myScale.available() == true)
  {
    currentReading = myScale.getReading();
    Serial.print("Reading: ");
    Serial.println(currentReading);
  }

  if (Serial.available())
  {
    byte incoming = Serial.read();

    if (incoming == 't')
      zeroOffset = currentReading; //Tare the scale
    else if (incoming == 'c')
      knownOffset = currentReading;
  }
}

//Record the current system settings to EEPROM
void recordSystemSettings(void)
{
  EEPROM.put(LOCATION_CALIBRATION_FACTOR, setting_calibration_factor);
  EEPROM.put(LOCATION_TARE_POINT, setting_tare_point);
}

//Reads the current system settings from EEPROM
//If anything looks weird, reset setting to default value
void readSystemSettings(void)
{
  //Look up the calibration factor
  EEPROM.get(LOCATION_CALIBRATION_FACTOR, setting_calibration_factor);
  if (setting_calibration_factor == 0xFFFFFFFF)
  {
    setting_calibration_factor = 0; //Default to 0
    EEPROM.put(LOCATION_CALIBRATION_FACTOR, setting_calibration_factor);
  }

  //Look up the zero tare point
  EEPROM.get(LOCATION_TARE_POINT, setting_tare_point);
  if (setting_tare_point == 0xFFFFFFFF)
  {
    setting_tare_point = 1000; //Default to 1000 so we don't get inf
    EEPROM.put(LOCATION_TARE_POINT, setting_tare_point);
  }
}
