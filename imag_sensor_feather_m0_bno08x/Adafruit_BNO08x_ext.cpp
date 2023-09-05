/* Adafruit_BNO08x_ext.cpp
 * 
 * extension of Adafruit's BNO08x sensor interface
 * part of imagination sensor firmware
 * 
 * 2021-05 rumori
 */

#include "Adafruit_BNO08x_ext.h"

Adafruit_BNO08x_ext::Adafruit_BNO08x_ext (int8_t reset_pin)
  : Adafruit_BNO08x (reset_pin)
{}

bool Adafruit_BNO08x_ext::softwareReset()
{
  return sh2_reinitialize() == SH2_OK;   
}


bool Adafruit_BNO08x_ext::setTare (uint8_t axes, sh2_TareBasis_t basis)
{
  return sh2_setTareNow (axes, basis) == SH2_OK;
}


bool Adafruit_BNO08x_ext::resetTare()
{
  return sh2_clearTare() == SH2_OK;  
}


bool Adafruit_BNO08x_ext::saveTare()
{
  return sh2_persistTare() == SH2_OK;
}


bool Adafruit_BNO08x_ext::saveDynamicCalibrationData()
{
  return sh2_saveDcdNow() == SH2_OK;
}


bool Adafruit_BNO08x_ext::setSensorsPerformingDynamicCalibration (uint8_t sensors)
{
  return sh2_setCalConfig (uint8_t (sensors)) == SH2_OK;
}


uint8_t Adafruit_BNO08x_ext::getSensorsPerformingDynamicCalibration()
{
  uint8_t sensors;

  return sh2_getCalConfig (&sensors) == SH2_OK ? sensors : 0;
}
