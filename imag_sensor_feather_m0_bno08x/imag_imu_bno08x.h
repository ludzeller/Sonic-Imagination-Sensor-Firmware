/* imag_imu_bno08x.h
 * 
 * imagination sensor firmware
 * hillcrest bno08x interface module
 * 
 * 2021-05 rumori
 */

#ifndef IMAG_IMU_BNO08X_H_INCLUDED
#define IMAG_IMU_BNO08X_H_INCLUDED

#include "imag_imu.h"

#include <Adafruit_BNO08x.h>

namespace Imag
{
class Imu_BNO08x : public Imu
{
  public:
    // Imu superclass interface
    bool init() override;
    bool queryData() override;
    bool getDataAsOsc (LiteOSCParser& osc) const override;

  private:
    // set which data the sensor should send
    bool setReports();
  
    // bno08x interface object
    Adafruit_BNO08x bno08x;

    // sensor value last read
    sh2_SensorValue_t sensorValue;

    // sensor value sequence
    /* one 8-bit number per available sensor return type
       wastes a few bytes as only a few types are used, but, well...
     */
    uint8_t sequence[SH2_MAX_SENSOR_ID];

}; // class Imu_BNO08x

} // namespace Imag

#endif // #ifndef IMAG_IMU_BNO08X_H_INCLUDED
 
