/* imag_imu_bno08x.cpp
 * 
 * imagination sensor firmware
 * hillcrest bno08x interface module
 * 
 * 2021-05 rumori
 */

#include "imag_imu_bno08x.h"

#if ! IMAG_IMU_DEBUG
#define DBG          ;
#define DBGLN        ;
#define DBGHEX       ;
#endif // #ifndef IMAG_IMU_DEBUG

using namespace Imag;

bool Imu_BNO08x::init()
{
  // init i2c, checks whether chip is present
  if (! bno08x.begin_I2C())
  {
    DBGLN("Imu_BNO08x: sensor not found");
    return false;
  }

  // enable reports
  if (! setReports())
  {
    DBGLN("Imu_BNO08x: could not set reports");
    return false;
  }

  return true;
}


bool Imu_BNO08x::queryData()
{
  // restore reports if sensor was reset
  if (bno08x.wasReset() && ! setReports())
  {
    DBGLN("Imu_BNO08x: setting reports after reset failed");
    return false;
  }

  // actually query sensor data
  if (! bno08x.getSensorEvent (&sensorValue))
  {
    DBGLN("Imu_BNO08x: querying sensor data failed");
    return false;
  }

  // TODO: check for sequence number gap

  // TODO: check for current accuracy

  // set data type
  switch (sensorValue.sensorId)
  {
    case SH2_ROTATION_VECTOR:
      type = ROTATION;
      break;

    case SH2_GEOMAGNETIC_ROTATION_VECTOR:
      type = ROTATION_GEO;
      break;

    case SH2_GAME_ROTATION_VECTOR:
      type = ROTATION_GAME;
      break;
      
    case SH2_LINEAR_ACCELERATION:
      type = ACCEL_LINEAR;
      break;

    default:
      type = NONE;
      DBGLN("Imu_BNO08x: received unknown report type");
      return false;
  }

  return true;
}


bool Imu_BNO08x::getDataAsOsc (LiteOSCParser& osc) const
{
  auto success = osc.init (oscAddr[type]);
  
  switch (type)
  {
    case ROTATION:
    case ROTATION_GEO:
    case ROTATION_GAME:
      success &= osc.addFloat (sensorValue.un.rotationVector.real);
      success &= osc.addFloat (sensorValue.un.rotationVector.i);
      success &= osc.addFloat (sensorValue.un.rotationVector.j);
      success &= osc.addFloat (sensorValue.un.rotationVector.k);
      break;

    case ACCEL_LINEAR:
      success &= osc.addFloat (sensorValue.un.linearAcceleration.x);
      success &= osc.addFloat (sensorValue.un.linearAcceleration.y);
      success &= osc.addFloat (sensorValue.un.linearAcceleration.z);
      break;

    default:
      DBGLN("Imu_BNO08x: cannot set osc data due to previously received unknown report type");
      return false;
  }

  return success;
}


bool Imu_BNO08x::setReports()
{
  if (! bno08x.enableReport (SH2_ROTATION_VECTOR))
    return false;

  if (! bno08x.enableReport (SH2_LINEAR_ACCELERATION))
    return false;

  return true;
}
