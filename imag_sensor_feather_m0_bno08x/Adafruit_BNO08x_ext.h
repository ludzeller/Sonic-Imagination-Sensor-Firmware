/* Adafruit_BNO08x_ext.h
 * 
 * extension of Adafruit's BNO08x sensor interface
 * part of imagination sensor firmware
 * 
 * 2021-2024 rumori
 */

#pragma once

#include <Adafruit_BNO08x.h>

class Adafruit_BNO08x_ext : public Adafruit_BNO08x
{
public:
    Adafruit_BNO08x_ext (int8_t reset_pin = -1);

    bool softwareReset();

    bool deviceReset();

    bool setTare (uint8_t axes, sh2_TareBasis_t basis);

    bool resetTare();
    
    bool saveTare();

    bool saveDynamicCalibrationData();
    
    bool clearDynamicCalibrationData();

    bool setSensorsPerformingDynamicCalibration (uint8_t sensors);

    bool getSensorsPerformingDynamicCalibration (uint8_t& sensors);

}; // class Adafruit_BNO08x_ext
