/* imag_imu_bno08x.cpp
 * 
 * imagination sensor firmware
 * hillcrest bno08x interface module
 * 
 * 2021-05 rumori
 */

#include "imag_imu_bno08x.h"
#include "imag_config.h"

#if ! IMAG_IMU_DEBUG
#define DBG          ;
#define DBGLN        ;
#define DBGHEX       ;
#endif // #if ! IMAG_IMU_DEBUG

using namespace Imag;


const std::array<int, static_cast<int> (Imu::DataType::total_num)> Imu_BNO08x::dataTypeToNativeIdMap
{
  SH2_ACCELEROMETER, // DataType::accel
  SH2_GYROSCOPE_CALIBRATED, // DataType::gyro
  SH2_MAGNETIC_FIELD_CALIBRATED, //dataTypeToNativeIdMap[static_cast<int> (DataType::mag)] = 
  SH2_ROTATION_VECTOR, // dataTypeToNativeIdMap[static_cast<int> (DataType::rotation)] = 
  SH2_GAME_ROTATION_VECTOR, // dataTypeToNativeIdMap[static_cast<int> (DataType::rotation_game)] = 
  SH2_GEOMAGNETIC_ROTATION_VECTOR, // dataTypeToNativeIdMap[static_cast<int> (DataType::rotation_geo)] = 
  SH2_ARVR_STABILIZED_RV, // dataTypeToNativeIdMap[static_cast<int> (DataType::rotation_arvr)] = 
  SH2_ARVR_STABILIZED_GRV, // dataTypeToNativeIdMap[static_cast<int> (DataType::rotation_game_arvr)] = 
  SH2_TAP_DETECTOR, // dataTypeToNativeIdMap[static_cast<int> (DataType::tap_detect)] = 
  SH2_STEP_DETECTOR, // dataTypeToNativeIdMap[static_cast<int> (DataType::step_detect)] = 
  SH2_STEP_COUNTER, // dataTypeToNativeIdMap[static_cast<int> (DataType::step_count)] = 
  SH2_SIGNIFICANT_MOTION, // dataTypeToNativeIdMap[static_cast<int> (DataType::significant_motion)] = 
  SH2_STABILITY_DETECTOR, // dataTypeToNativeIdMap[static_cast<int> (DataType::stability_detect)] = 
  SH2_STABILITY_CLASSIFIER, // dataTypeToNativeIdMap[static_cast<int> (DataType::stability_class)] = 
  SH2_PERSONAL_ACTIVITY_CLASSIFIER, // dataTypeToNativeIdMap[static_cast<int> (DataType::activity_class)] = ;
  SH2_SHAKE_DETECTOR // dataTypeToNativeIdMap[static_cast<int> (DataType::shake_detect)] = 
};


Imu_BNO08x::Imu_BNO08x()
  : bno08x (Config::bno08x_reset_pin),
    reliability (0),
    accuracy (-1.0f),
    sourceOfReliability (DataType::none),
    sourceOfAccuracy (DataType::none),
    calibrating (false)
{
  // interrupt pin, only configure for now (unused)
  pinMode (Config::bno08x_int_pin, INPUT_PULLUP);

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

  // set sensors which do auto-calibration
  // putting this in reinit() apparently crashes the sensor when done repeatedly, strange...
  if (! setDefaultAutoCalibration())
  {
    DBGLN("Imu_BNO08x: error setting default auto calibration sensors");
    return false;
  }

  return reinit();
}


bool Imu_BNO08x::available()
{
  // restore reports if sensor was reset
  if (bno08x.wasReset() && ! reinit())
  {
    DBGLN("Imu_BNO08x: reinit after reset failed");
    return false;
  }

  // actually query sensor data, returns false if none available
  if (! bno08x.getSensorEvent (&sensorValue))
    return false;

  return true;
}


bool Imu_BNO08x::read()
{
  // TODO: check for sequence number gap

  // set data type
  if ((lastType = nativeIdToDataType (sensorValue.sensorId)) == DataType::none)
  {
    DBGLN("Imu_BNO08x: received unknown report type");
    return false;
  }

  // store reliability
  if (lastType == sourceOfReliability)
    reliability = sensorValue.status & 0x03;

  // store accuracy
  if (lastType == sourceOfAccuracy)
  {
    if (lastType == DataType::rotation ||
        lastType == DataType::rotation_geo ||
        lastType == DataType::rotation_arvr)
      accuracy = sensorValue.un.rotationVector.accuracy;
    else
      accuracy = -1.0f;
  }

  return true;
}


bool Imu_BNO08x::getLastData (Quaternion& rotation)
{
    if (! isAnyRotationDataType (lastType))
        return false;

    rotation.w = sensorValue.un.rotationVector.real;
    rotation.x = sensorValue.un.rotationVector.i;
    rotation.y = sensorValue.un.rotationVector.j;
    rotation.z = sensorValue.un.rotationVector.k;

    return true;
}


bool Imu_BNO08x::setReorientation (const Quaternion& newReorientation)
{
    reorientation = newReorientation;
    return updateReorientation();
}


bool Imu_BNO08x::beginCalibration()
{
  auto res = disableAllSensors();

  // enable dynamic calibration according to BNO08x Sensor Calibration Procedure document
  res &= bno08x.setSensorsPerformingDynamicCalibration (SH2_CAL_ACCEL | SH2_CAL_GYRO | SH2_CAL_MAG);

  // enable only sensors according to BNO08x Sensor Calibration Procedure document
  if (! (bno08x.enableReport (SH2_GAME_ROTATION_VECTOR) &&            // game rotation$
         bno08x.enableReport (SH2_MAGNETIC_FIELD_CALIBRATED, 20000))) // magnetic field @ 50Hz
  {
    res = false;
    DBGLN("Imu_BNO08x: error enabling sensor reports for calibration");
  }

  // get reliability from magnetic field
  sourceOfReliability = DataType::mag;

  if (res)
    calibrating = true;
  
  return res;
}


bool Imu_BNO08x::endCalibration()
{
  calibrating = false;

  // set back to default mode
  return setDefaultAutoCalibration() && reinit();
}


bool Imu_BNO08x::saveCalibration()
{
  if (! bno08x.saveDynamicCalibrationData())
  {
    DBGLN("Imu_BNO08x: error saving dynamic calibration data");
    return false;
  }

  return true;
}

void Imu_BNO08x::printCalibrationReliability()
{
#if IMAG_IMU_DEBUG
  DBG("Imu_BNO08x: current calibration reliability: ");
  switch (reliability)
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
#endif // IMAG_IMU_DEBUG
}


bool Imu_BNO08x::printSensorsPerformingDynamicCalibration()
{
#if IMAG_IMU_DEBUG
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
#endif // IMAG_IMU_DEBUG

  return true;
}


bool Imu_BNO08x::reinit()
{
  initialised = false;

  // enable reports
  if (! updateDataTypesToQuery())
  {
    DBGLN("Imu_BNO08x: could not set reports");
    return false;
  }

  // set reorientation
  if (! updateReorientation())
  {
    DBGLN("Imu_BNO08x: error setting reorientation to sensor");
    return false;
  }

  return initialised = true;
}


bool Imu_BNO08x::updateDataTypesToQuery (const std::vector<Imu::DataType>& newTypesToQuery)
{
  auto res = true;

  // disable all sensors
  res = disableAllSensors();

  // enable requested sensors
  for (auto type : newTypesToQuery)
  {
    if (! bno08x.enableReport (dataTypeToNativeId (type), 1000000 / queryRates[static_cast<int> (type)]))
    {
      DBG("Imu_BNO08x: error while enabling sensor for data type "); DBGLN(static_cast<int> (type));
      res = false;
    }
  }

  // get reliability and accuracy from first type in query list
  sourceOfReliability = newTypesToQuery.size() > 0 ? newTypesToQuery[0] : DataType::none;
  sourceOfAccuracy = sourceOfReliability;

  return res;
}


bool Imu_BNO08x::disableAllSensors()
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

  return res;
}


bool Imu_BNO08x::setTare (bool tareFull)
{
  sh2_TareBasis_t basis = SH2_TARE_BASIS_ROTATION_VECTOR; // default
  uint8_t axes = SH2_TARE_Z;

  if (typesToQuery.size() > 0)
  {
    if (typesToQuery[0] == DataType::rotation_game)
      basis = SH2_TARE_BASIS_GAMING_ROTATION_VECTOR;
    else if (typesToQuery[0] == DataType::rotation_geo)
      basis = SH2_TARE_BASIS_GEOMAGNETIC_ROTATION_VECTOR;
  }

  if (tareFull)
    axes = SH2_TARE_X | SH2_TARE_Y | SH2_TARE_Z;

  if (! bno08x.setTare (axes, basis))
  {
    DBGLN("Imu_BNO08x: error setting tare");
    return false;
  }

  return true;
}


bool Imu_BNO08x::setDefaultAutoCalibration()
{
  // according to recommendation in BNO08x Sensor Calibration Procedure document
  return bno08x.setSensorsPerformingDynamicCalibration (SH2_CAL_ACCEL);
}


bool Imu_BNO08x::updateReorientation()
{
    auto sh2Quat = quatToSh2Quat (reorientation);
    
    return sh2_setReorientation (&sh2Quat) == SH2_OK;
}
