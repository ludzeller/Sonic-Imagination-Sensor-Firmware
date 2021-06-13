/* Adafruit_BNO08x_ext.h
 * 
 * extension of Adafruit's BNO08x sensor interface
 * part of imagination sensor firmware
 * 
 * 2021-05 rumori
 */

#ifndef ADADFRUIT_BNO08x_EXT_H_INCLUDED
#define ADADFRUIT_BNO08x_EXT_H_INCLUDED

#include <Adafruit_BNO08x.h>

class Adafruit_BNO08x_ext : public Adafruit_BNO08x
{
  public:  
    bool softwareReset();

    bool saveDynamicCalibrationData();
    
    bool setSensorsPerformingDynamicCalibration (uint8_t sensors);

    uint8_t getSensorsPerformingDynamicCalibration();


};

#endif // #ifndef ADADFRUIT_BNO08x_EXT_H_INCLUDED
