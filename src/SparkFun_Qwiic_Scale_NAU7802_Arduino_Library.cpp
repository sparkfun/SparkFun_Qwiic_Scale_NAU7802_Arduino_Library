/*
  This is an Arduino library written for the NAU7802 24-bit wheatstone
  bridge and load cell amplifier.
  By Nathan Seidle @ SparkFun Electronics, March 3nd, 2019

  The NAU7802 is an I2C device that converts analog signals to a 24-bit
  digital signal. This makes it possible to create your own digital scale
  either by hacking an off-the-shelf bathroom scale or by creating your
  own scale using a load cell.

  The NAU7802 is a better version of the popular HX711 load cell amplifier.
  It uses a true I2C interface so that it can share the bus with other
  I2C devices while still taking very accurate 24-bit load cell measurements
  up to 320Hz.

  https://github.com/sparkfun/SparkFun_Qwiic_Scale_NAU7802_Arduino_Library

  SparkFun labored with love to create this code. Feel like supporting open
  source? Buy a board from SparkFun!
  https://www.sparkfun.com/products/15242
*/

#include "SparkFun_Qwiic_Scale_NAU7802_Arduino_Library.h"

//Constructor
NAU7802::NAU7802()
{
}

//Sets up the NAU7802 for basic function
//Returns true upon completion
bool NAU7802::begin(TwoWire &wirePort)
{
  //Get user's options
  _i2cPort = &wirePort;

  //Check if the device ack's over I2C
  if (isConnected() == false)
  {
    //There are rare times when the sensor is occupied and doesn't ack. A 2nd try resolves this.
    if (isConnected() == false)
      return (false);
  }

  bool result = true; //Accumulate a result as we do the setup

  result &= reset(); //Reset all registers

  result &= powerUp(); //Power on analog and digital sections of the scale

  result &= setLDO(NAU7802_LDO_3V3); //Set LDO to 3.3V

  result &= setGain(NAU7802_GAIN_16); //Set gain to 16

  result &= setSampleRate(NAU7802_SPS_10); //Set samples per second to 10

  result &= setRegister(NAU7802_ADC, 0x30); //Turn off CLK_CHP. From 9.1 power on sequencing.

  result &= calibrate();

  return (result);
}

//Returns true if device is present
//Tests for device ack to I2C address
bool NAU7802::isConnected()
{
  _i2cPort->beginTransmission(_deviceAddress);
  if (_i2cPort->endTransmission() != 0)
    return (false); //Sensor did not ACK
  return (true);    //All good
}

//Returns true if Cycle Ready bit is set (conversion is complete)
bool NAU7802::available()
{
  return (getBit(NAU7802_PU_CTRL_CR, NAU7802_PU_CTRL));
}

//Calibrate system. Returns true if CAL_ERR bit is 0 (no error)
//Takes approximately 344ms to calibrate
bool NAU7802::calibrate()
{
  setBit(NAU7802_CTRL2_CALS, NAU7802_CTRL2); //Begin calibration

  uint16_t counter = 0;
  while (1)
  {
    if (getBit(NAU7802_CTRL2_CALS, NAU7802_CTRL2) == false)
      break; //Goes to 0 once cal is complete
    delay(1);
    if (counter++ > 1000)
      return (false);
  }

  if (getBit(NAU7802_CTRL2_CAL_ERROR, NAU7802_CTRL2) == false)
    return (true); //No error! Cal is good.
  return (false);  //Cal error
}

//Set the readings per second
//10, 20, 40, 80, and 320 samples per second is available
bool NAU7802::setSampleRate(uint8_t rate)
{
  if (rate > 0b111)
    rate = 0b111; //Error check

  uint8_t value = getRegister(NAU7802_CTRL2);
  value &= 0b10001111; //Clear CRS bits
  value |= rate << 4;  //Mask in new CRS bits

  return (setRegister(NAU7802_CTRL2, value));
}

//Select between 1 and 2
bool NAU7802::setChannel(uint8_t channelNumber)
{
  if (channelNumber == NAU7802_CHANNEL_1)
    return (clearBit(NAU7802_CTRL2_CHS, NAU7802_CTRL2)); //Channel 1 (default)
  else
    return (setBit(NAU7802_CTRL2_CHS, NAU7802_CTRL2)); //Channel 2
}

//Power up digital and analog sections of scale
bool NAU7802::powerUp()
{
  setBit(NAU7802_PU_CTRL_PUD, NAU7802_PU_CTRL);
  setBit(NAU7802_PU_CTRL_PUA, NAU7802_PU_CTRL);

  //Wait for Power Up bit to be set - takes approximately 200us
  uint8_t counter = 0;
  while (1)
  {
    if (getBit(NAU7802_PU_CTRL_PUR, NAU7802_PU_CTRL) == true)
      break; //Good to go
    delay(1);
    if (counter++ > 100)
      return (false); //Error
  }
  return (true);
}

//Puts scale into low-power mode
bool NAU7802::powerDown()
{
  clearBit(NAU7802_PU_CTRL_PUD, NAU7802_PU_CTRL);
  return (clearBit(NAU7802_PU_CTRL_PUA, NAU7802_PU_CTRL));
}

//Resets all registers to Power Of Defaults
bool NAU7802::reset()
{
  setBit(NAU7802_PU_CTRL_RR, NAU7802_PU_CTRL); //Set RR
  delay(1);
  return (clearBit(NAU7802_PU_CTRL_RR, NAU7802_PU_CTRL)); //Clear RR to leave reset state
}

//Set the onboard Low-Drop-Out voltage regulator to a given value
//2.4, 2.7, 3.0, 3.3, 3.6, 3.9, 4.2, 4.5V are available
bool NAU7802::setLDO(uint8_t ldoValue)
{
  if (ldoValue > 0b111)
    ldoValue = 0b111; //Error check

  //Set the value of the LDO
  uint8_t value = getRegister(NAU7802_CTRL1);
  value &= 0b11000111;    //Clear LDO bits
  value |= ldoValue << 3; //Mask in new LDO bits
  setRegister(NAU7802_CTRL1, value);

  return (setBit(NAU7802_PU_CTRL_AVDDS, NAU7802_PU_CTRL)); //Enable the internal LDO
}

//Set the gain
//x1, 2, 4, 8, 16, 32, 64, 128 are avaialable
bool NAU7802::setGain(uint8_t gainValue)
{
  if (gainValue > 0b111)
    gainValue = 0b111; //Error check

  uint8_t value = getRegister(NAU7802_CTRL1);
  value &= 0b11111000; //Clear gain bits
  value |= gainValue;  //Mask in new bits

  return (setRegister(NAU7802_CTRL1, value));
}

//Get the revision code of this IC
uint8_t NAU7802::getRevisionCode()
{
  uint8_t revisionCode = getRegister(NAU7802_DEVICE_REV);
  return (revisionCode & 0x0F);
}

//Returns 24-bit reading
//Assumes CR Cycle Ready bit (ADC conversion complete) has been checked to be 1
uint32_t NAU7802::getReading()
{
  _i2cPort->beginTransmission(_deviceAddress);
  _i2cPort->write(NAU7802_ADCO_B2);
  if (_i2cPort->endTransmission() != 0)
    return (false); //Sensor did not ACK

  _i2cPort->requestFrom((uint8_t)_deviceAddress, (uint8_t)3);

  if (_i2cPort->available())
  {
    uint32_t value = (uint32_t)_i2cPort->read() << 16; //MSB
    value |= (uint32_t)_i2cPort->read() << 8;          //MidSB
    value |= (uint32_t)_i2cPort->read();               //LSB
    return (value);
  }

  return (0); //Error
}

//Return the average of a given number of readings
//Gives up after 1000ms so don't call this function to average 8 samples setup at 1Hz output (requires 8s)
uint32_t NAU7802::getAverage(uint8_t averageAmount)
{
  unsigned long total = 0;
  uint8_t samplesAquired = 0;

  unsigned long startTime = millis();
  while (1)
  {
    if (available() == true)
    {
      total += getReading();
      if (samplesAquired++ == averageAmount)
        break; //All done
    }
    if (millis() - startTime > 1000)
      return (0); //Timeout - Bail with error
  }
  total /= averageAmount;

  return (total);
}

//Call when scale is setup, level, at running temperature, with nothing on it
void NAU7802::calculateZeroOffset(uint8_t averageAmount)
{
  setZeroOffset(getAverage(averageAmount));
}

//Sets the internal variable. Useful for users who are loading values from NVM.
void NAU7802::setZeroOffset(uint32_t newZeroOffset)
{
  _zeroOffset = newZeroOffset;
}

uint32_t NAU7802::getZeroOffset()
{
  return (_zeroOffset);
}

//Call after zeroing. Provide the float weight sitting on scale. Units do not matter.
void NAU7802::calculateCalibrationFactor(float weightOnScale, uint8_t averageAmount)
{
  uint32_t onScale = getAverage(8);
  float newCalFactor = (onScale - _zeroOffset) / (float)weightOnScale;
  setCalibrationFactor(newCalFactor);
}

//Pass a known calibration factor into library. Helpful if users is loading settings from NVM.
//If you don't know your cal factor, call setZeroOffset(), then calculateCalibrationFactor() with a known weight
void NAU7802::setCalibrationFactor(float newCalFactor)
{
  _calibrationFactor = newCalFactor;
}

float NAU7802::getCalibrationFactor()
{
  return (_calibrationFactor);
}

//Returns the y of y = mx + b using the current weight on scale, the cal factor, and the offset.
float NAU7802::getWeight(bool allowNegativeWeights)
{
  uint32_t onScale = getAverage(8);

  //Prevent the current reading from being less than zero offset
  //This happens when the scale is zero'd, unloaded, and the load cell reports a value slightly less than zero value
  //causing the weight to be negative or jump to millions of pounds
  if (allowNegativeWeights == false)
  {
    if (onScale < _zeroOffset)
      onScale = _zeroOffset; //Force reading to zero
  }

  float weight = (onScale - _zeroOffset) / _calibrationFactor;
  return (weight);
}

//Set Int pin to be high when data is ready (default)
bool NAU7802::setIntPolarityHigh()
{
  return (clearBit(NAU7802_CTRL1_CRP, NAU7802_CTRL1)); //0 = CRDY pin is high active (ready when 1)
}

//Set Int pin to be low when data is ready
bool NAU7802::setIntPolarityLow()
{
  return (setBit(NAU7802_CTRL1_CRP, NAU7802_CTRL1)); //1 = CRDY pin is low active (ready when 0)
}

//Mask & set a given bit within a register
bool NAU7802::setBit(uint8_t bitNumber, uint8_t registerAddress)
{
  uint8_t value = getRegister(registerAddress);
  value |= (1 << bitNumber); //Set this bit
  return (setRegister(registerAddress, value));
}

//Mask & clear a given bit within a register
bool NAU7802::clearBit(uint8_t bitNumber, uint8_t registerAddress)
{
  uint8_t value = getRegister(registerAddress);
  value &= ~(1 << bitNumber); //Set this bit
  return (setRegister(registerAddress, value));
}

//Return a given bit within a register
bool NAU7802::getBit(uint8_t bitNumber, uint8_t registerAddress)
{
  uint8_t value = getRegister(registerAddress);
  value &= (1 << bitNumber); //Clear all but this bit
  return (value);
}

//Get contents of a register
uint8_t NAU7802::getRegister(uint8_t registerAddress)
{
  _i2cPort->beginTransmission(_deviceAddress);
  _i2cPort->write(registerAddress);
  if (_i2cPort->endTransmission() != 0)
    return (-1); //Sensor did not ACK

  _i2cPort->requestFrom((uint8_t)_deviceAddress, (uint8_t)1);

  if (_i2cPort->available())
    return (_i2cPort->read());

  return (-1); //Error
}

//Send a given value to be written to given address
//Return true if successful
bool NAU7802::setRegister(uint8_t registerAddress, uint8_t value)
{
  _i2cPort->beginTransmission(_deviceAddress);
  _i2cPort->write(registerAddress);
  _i2cPort->write(value);
  if (_i2cPort->endTransmission() != 0)
    return (false); //Sensor did not ACK
  return (true);
}
