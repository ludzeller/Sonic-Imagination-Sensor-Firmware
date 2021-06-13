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
  bool read() override { return true; }; // already done by available with this sensor
  bool getDataAsOsc (LiteOSCParser& osc) const override;
  
  bool beginCalibration() override;
  bool endCalibration() override;
  bool isCalibrating() const override { return calibrating; }

  // bool setDataRate (int rate) override;
  float getCurrentReliability() const override { return reliability / 3.0f; }

  void printCalibrationReliability();
  bool printSensorsPerformingDynamicCalibration();

private:
  bool updateDataTypesToQuery() override { return updateDataTypesToQuery (typesToQuery); }
  bool updateDataTypesToQuery (const std::vector<DataType>& newTypesToQuery);
  bool disableAllSensors();

  const std::array<int, static_cast<int> (DataType::total_num)>& getDataTypeToNativeIdMap() const override { return dataTypeToNativeIdMap; }

  // set default auto calibration mode
  bool setDefaultAutoCalibration();

  // maps Imu::DataType to native sensor ids
  static void initDataTypeToNativeIdMap();
  static std::array<int, static_cast<int> (DataType::total_num)> dataTypeToNativeIdMap;

  // bno08x interface object
  Adafruit_BNO08x_ext bno08x;

  // sensor value last read
  sh2_SensorValue_t sensorValue;

  // query rates per data type
  std::array<int, static_cast<int> (DataType::total_num)> queryRates;

  // reliability sent with last matching sensor data
  int reliability;

  // sensor report type from which to set reliability member
  DataType sourceOfReliability;

  // sensor value sequence
  /* one 8-bit number per available sensor return type
     wastes a few bytes as only a few types are used, but, well...
  */
  std::array<uint8_t, SH2_MAX_SENSOR_ID> sequenceNumbers;

  // calibration mode flag
  bool calibrating;

}; // class Imu_BNO08x

} // namespace Imag
