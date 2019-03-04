/*
  Use the Qwiic Scale to read load cells and scales
  By: Nathan Seidle @ SparkFun Electronics
  Date: March 3rd, 2019
  License: This code is public domain but you buy me a beer if you use this 
  and we meet someday (Beerware license).

  The Qwiic Scale is an I2C device that converts analog signals to a 24-bit
  digital signal. This makes it possible to create your own digital scale
  either by hacking an off-the-shelf bathroom scale or by creating your
  own scale using a load cell.
  
  SparkFun labored with love to create this code. Feel like supporting open
  source? Buy a board from SparkFun!
  https://www.sparkfun.com/products/15242
 
  example showing the use of Int pin
  save scale / cal / offset to NVM in Uno
*/

#include <Wire.h>

#include "SparkFun_Qwiic_Scale_NAU7802_Arduino_Library.h" // Click here to get the library: http://librarymanager/All#SparkFun_NAU8702

NAU7802 myScale; //Create instance of the NAU7802 class

long samples = 0;

long zeroOffset; //This is the reading when there is nothing on the scale
long knownOffset; //This is the reading when there is a known weight on the scale
long currentReading; //This is the lastest reading from the sensor

void setup()
{
  Serial.begin(115200);
  Serial.println("Qwiic Scale Example");

  Wire.begin();
  Wire.setClock(400000);

  long startTime = millis();
  if (myScale.begin() == false)
  {
    Serial.println("Scale not detected. Please check wiring. Freezing...");
    while (1);
  }
  Serial.println("Scale detected!");

  //myScale.getReading(); //Returns a 24-bit number. This number is unit-less and needs to be scaled to a known weight.
  //myScale.available(); //Goes true once conversion is complete
  //myScale.calibrate(); //Does an internal calibration. Recommended after power up, gain changes, sample rate changes, or channel changes.
  //myScale.setGain(NAU7802_GAIN_16); //Gain can be set from 1 to 128.
  //myScale.setSampleRate(NAU7802_SPS_10); //NAU7802 can output data from 10Hz to a max of 320Hz.

  //myScale.powerUp(); //Use this function to bring the NAU7802 out of power down mode.
  //myScale.powerDown(); //NAU7802 uses ~2mA powered up and ~200nA powered down.

  //myScale.reset(); //Reset all registers to power on default state
  //myScale.setLDO(NAU7802_LDO_3V3); //The Qwiic Scale operates at 3.3V so we lower the LDO to this value. If you're using the IC in a different setup, 2.4 to 4.5V is available.
  //myScale.setChannel(NAU7802_CHANNEL_1); //There are two available channels, but channel 1 is all you need for scales.

  //myScale.setIntPolarityHigh(); //Set Int pin to be high when data is ready (default)
  //myScale.setIntPolarityLow(); //Set Int pin to be low when data is ready

  //byte revisionCode = myScale.getRevisionCode();
  //Serial.print("Revision code: 0x");
  //Serial.println(revisionCode, HEX);
  //while(1);

  while(myScale.available() == false) ; //Wait for first reading to come in

  Serial.print("Startup time: ");
  Serial.print(millis() - startTime);
  Serial.println("ms");
}

void loop()
{
  if(myScale.available() == true)
  {
    samples++;
    Serial.print(samples / (millis() / 1000.0), 2);
    Serial.print("Hz,");
    currentReading = myScale.getReading();
    Serial.print("Reading: ");
    Serial.println(currentReading);
  }

  if(Serial.available())
  {
    byte incoming = Serial.read();

    if(incoming == 't')
      zeroOffset = currentReading; //Tare the scale
    else if(incoming == 'c')
      knownOffset = currentReading;      
  }
}
