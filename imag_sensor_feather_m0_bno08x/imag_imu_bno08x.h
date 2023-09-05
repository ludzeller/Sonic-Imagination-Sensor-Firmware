/* imag_imu_bno08x.h
 * 
 * imagination sensor firmware
 * hillcrest bno08x interface module
 * 
 * 2021-05 rumori
 */

#pragma once

#include "imag_imu.h"
#include "Adafruit_BNO08x_ext.h"

namespace Imag
{
class Imu_BNO08x : public Imu
{
public:
  Imu_BNO08x();
  
  // Imu superclass interface
  bool init() override;
  bool available() override;
  bool read() override;
  bool getLastData (Quaternion& rotation) override;

  bool setTareFull() override { return setTare (true); }
  bool setTareHeading() override { return setTare(); }
  bool resetTare() override { return updateReorientation(); }
  
  bool setReorientation (const Quaternion& newReorientation) override;

  bool beginCalibration() override;
  bool endCalibration() override;
  bool saveCalibration() override;
  bool isCalibrating() const override { return calibrating; }

  // bool setDataRate (int rate) override;
  float getCurrentReliability() const override { return reliability / 3.0f; }
  float getCurrentAccuracy() const override { return accuracy; }

  // debug printers
  void printCalibrationReliability();
  bool printSensorsPerformingDynamicCalibration();

private:
  bool reinit();
  bool updateDataTypesToQuery() override { return updateDataTypesToQuery (typesToQuery); }
  bool updateDataTypesToQuery (const std::vector<DataType>& newTypesToQuery);
  bool disableAllSensors();
  bool setTare (bool tareFull = false);

  const std::array<int, static_cast<int> (DataType::total_num)>& getDataTypeToNativeIdMap() const override { return dataTypeToNativeIdMap; }

  // set default auto calibration mode
  bool setDefaultAutoCalibration();

  // send current reorientation to sensor
  bool updateReorientation();

  // quaternion translation helpers
  static sh2_Quaternion_t quatToSh2Quat (const Quaternion& quat) { return { quat.x, quat.y, quat.z, quat.w }; }
  static Quaternion sh2QuatToQuat (const sh2_Quaternion_t& sh2Quat) { return { float (sh2Quat.w), float (sh2Quat.x), float (sh2Quat.y), float (sh2Quat.z) }; }

  // bno08x interface object
  Adafruit_BNO08x_ext bno08x;

  // sensor value last read
  sh2_SensorValue_t sensorValue;

  // maps Imu::DataType to native sensor ids
  static const std::array<int, static_cast<int> (DataType::total_num)> dataTypeToNativeIdMap;

  // query rates per data type
  std::array<int, static_cast<int> (DataType::total_num)> queryRates;

  // reliability sent with last matching sensor data
  int reliability;

  // accuracy sent with last sensor data or negative if invalid
  float accuracy;

  // sensor report type from which to set reliability member
  DataType sourceOfReliability;

  // sensor report type from which to set accuracy member (if supported)
  DataType sourceOfAccuracy;

  // sensor value sequence
  /* one 8-bit number per available sensor return type
     wastes a few bytes as only a few types are used, but, well...
  */
  std::array<uint8_t, SH2_MAX_SENSOR_ID> sequenceNumbers;

  // calibration mode flag
  bool calibrating;

}; // class Imu_BNO08x

} // namespace Imag
