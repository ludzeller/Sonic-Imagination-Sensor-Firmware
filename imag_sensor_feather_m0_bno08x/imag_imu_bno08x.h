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
  bool getDataAsOsc (LiteOSCParser& osc) const override;

  // cleanup!
  bool sendMidi();

  bool setTareFull() override { return setTare (true); }
  bool setTareHeading() override { return setTare(); }
  // bool setTareTilt() override;
  bool resetTare() override { return updateReorientation(); }
  bool saveTare() override { return bno08x.saveTare(); }
  bool setReorientation (double x, double y, double z, double w) override;

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

  // reorientation quaternion
  sh2_Quaternion_t reorientation;

}; // class Imu_BNO08x

} // namespace Imag
