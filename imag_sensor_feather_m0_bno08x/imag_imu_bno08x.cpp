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
#endif // #if ! IMAG_IMU_DEBUG

using namespace Imag;

Imu_BNO08x::Imu_BNO08x()
{
  initDataTypeToNativeIdMap(); // would suffice on first instantiation
  queryRates.fill (100); // default
}


bool Imu_BNO08x::init()
{
  // init i2c, checks whether chip is present
  if (! bno08x.begin_I2C())
  {
    DBGLN("Imu_BNO08x: sensor not found");
    return false;
  }

  // enable reports
  if (! updateDataTypesToQuery())
  {
    DBGLN("Imu_BNO08x: could not set reports");
    return false;
  }

  return true;
}


bool Imu_BNO08x::available()
{
  // restore reports if sensor was reset
  if (bno08x.wasReset() && ! updateDataTypesToQuery())
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

  // set data type
  if ((lastType = nativeIdToDataType (sensorValue.sensorId)) == DataType::none)
  {
    DBGLN("Imu_BNO08x: received unknown report type");
    return false;
  }

  return true;
}


bool Imu_BNO08x::getDataAsOsc (LiteOSCParser& osc) const
{
  auto success = osc.init (oscAddr[static_cast<int> (lastType)]);
  
  switch (lastType)
  {
    case DataType::rotation:
    case DataType::rotation_geo:
    case DataType::rotation_game:
      success &= osc.addFloat (sensorValue.un.rotationVector.real);
      success &= osc.addFloat (sensorValue.un.rotationVector.i);
      success &= osc.addFloat (sensorValue.un.rotationVector.j);
      success &= osc.addFloat (sensorValue.un.rotationVector.k);
      break;

    default:
      DBGLN("Imu_BNO08x: cannot set osc data due to previously received unknown report type");
      return false;
  }

  return success;
}


void Imu_BNO08x::printCalibrationReliability()
{
  DBG("Imu_BNO08x: current calibration reliability: ");
  switch (sensorValue.status & 0x03)
  {
    case 0:
      DBGLN("Unreliable");
      break;

    case 1:
      DBGLN("Low");
      break;
    
    case 2:
      DBGLN("Medium");
      break;

    case 3:
      DBGLN("High");
      break;
  }
}


bool Imu_BNO08x::printSensorsPerformingDynamicCalibration()
{
  auto sensors = bno08x.getSensorsPerformingDynamicCalibration();

  if (! sensors)
  {
    DBGLN("Imu_BNO08x: querying sensor dynamic calibration state failed");
    return false;
  }

  DBGLN("Imu_BNO08x: sensors performing dynamic calibration:");
  DBG("Imu_BNO08x: Accelerometer: "); DBGLN(sensors & SH2_CAL_ACCEL  ? "on" : "off");
  DBG("Imu_BNO08x: Gyroscope    : "); DBGLN(sensors & SH2_CAL_GYRO   ? "on" : "off");
  DBG("Imu_BNO08x: Magnetometer : "); DBGLN(sensors & SH2_CAL_MAG    ? "on" : "off");
  DBG("Imu_BNO08x: Planar       : "); DBGLN(sensors & SH2_CAL_PLANAR ? "on" : "off");

  return true;
}


bool Imu_BNO08x::updateDataTypesToQuery (const std::vector<Imu::DataType>& newTypesToQuery)
{
  auto res = true;

  // disable all sensors
  for (auto type = 0; type < static_cast<int> (DataType::total_num); ++type)
  {
    if (! bno08x.enableReport (dataTypeToNativeId (static_cast<DataType> (type)), 0))
    {
      DBG("Imu_BNO08x: error while disabling sensor for data type "); DBGLN(type);
      res = false;
    }
  }

  // enable requested sensors
  for (auto type : newTypesToQuery)
  {
    if (! bno08x.enableReport (dataTypeToNativeId (type), queryRates[static_cast<int> (type)]))
    {
      DBG("Imu_BNO08x: error while enabling sensor for data type "); DBGLN(static_cast<int> (type));
      res = false;
    }
  }

  return res;
}


std::array<int, static_cast<int> (Imu::DataType::total_num)> Imu_BNO08x::dataTypeToNativeIdMap { };

void Imu_BNO08x::initDataTypeToNativeIdMap()
{
  dataTypeToNativeIdMap.fill (-1); // not supported by default
  
  dataTypeToNativeIdMap[static_cast<int> (DataType::accel)] = SH2_ACCELEROMETER;
  dataTypeToNativeIdMap[static_cast<int> (DataType::gyro)] = SH2_GYROSCOPE_CALIBRATED;
  dataTypeToNativeIdMap[static_cast<int> (DataType::mag)] = SH2_MAGNETIC_FIELD_CALIBRATED;
  dataTypeToNativeIdMap[static_cast<int> (DataType::rotation)] = SH2_ROTATION_VECTOR;
  dataTypeToNativeIdMap[static_cast<int> (DataType::rotation_game)] = SH2_GAME_ROTATION_VECTOR;
  dataTypeToNativeIdMap[static_cast<int> (DataType::rotation_geo)] = SH2_GEOMAGNETIC_ROTATION_VECTOR;
  dataTypeToNativeIdMap[static_cast<int> (DataType::tap_detect)] = SH2_TAP_DETECTOR;
  dataTypeToNativeIdMap[static_cast<int> (DataType::step_detect)] = SH2_STEP_DETECTOR;
  dataTypeToNativeIdMap[static_cast<int> (DataType::step_count)] = SH2_STEP_COUNTER;
  dataTypeToNativeIdMap[static_cast<int> (DataType::significant_motion)] = SH2_SIGNIFICANT_MOTION;
  dataTypeToNativeIdMap[static_cast<int> (DataType::stability_detect)] = SH2_STABILITY_DETECTOR;
  dataTypeToNativeIdMap[static_cast<int> (DataType::stability_class)] = SH2_STABILITY_CLASSIFIER;
  dataTypeToNativeIdMap[static_cast<int> (DataType::activity_class)] = SH2_PERSONAL_ACTIVITY_CLASSIFIER;
  dataTypeToNativeIdMap[static_cast<int> (DataType::shake_detect)] = SH2_SHAKE_DETECTOR;
}
