/* imag_imu.h
 * 
 * imagination sensor firmware
 * generic imu interface
 * 
 * 2021-05 rumori
 */

#pragma once

#include <array>
#include <vector>

#include <Arduino_Helpers.h>
#include <AH/Math/Quaternion.hpp>

#include <LiteOSCParser.h>
using LiteOSCParser = qindesign::osc::LiteOSCParser;

#include "imag_debug.h"

namespace Imag
{
namespace OscAddress
{
  static constexpr auto none       { "/invalid" };
  static constexpr auto rotation   { "/rot" };  // rotation as a quaternion: 4 floats [ i, j, k, r ]
}

class Imu
{
public:
  // data types that can be queried from sensor
  enum class DataType
    {
      none  = -1,
      accel = 0,
      gyro,
      mag,
      rotation,
      rotation_game,
      rotation_geo,
      rotation_arvr,
      rotation_game_arvr,
      tap_detect,
      step_detect,
      step_count,
      significant_motion,
      stability_detect,
      stability_class,
      activity_class,
      shake_detect,

      total_num,
      max_ever_num = 64
    };

  // osc address to be used for each data type
  static const std::array<const char* const, static_cast<int> (DataType::total_num)> oscAddr;

  // convenience method for datatype classes
  static bool isAnyRotationDataType (const DataType& dataType)
  {
      auto intType = static_cast<int> (dataType);

      return intType >= static_cast<int> (DataType::rotation) && intType <= static_cast<int> (DataType::rotation_game_arvr);
  }

  // constructor
  Imu();

  // check sensor presence and initialise 
  virtual bool init() = 0;

  // check if data is available
  virtual bool available() = 0;

  // query data from sensor if available and set type member
  virtual bool read() = 0;

  // return previously received data
  virtual bool getLastData (Quaternion& rotation) = 0;

  // get type of previously queried data
  DataType getLastDataType() const { return lastType; }

  // set data types to query from sensor
  bool setDataTypesToQuery (const std::initializer_list<DataType>& dataTypes);

  // get data types currently queried by sensor
  const std::vector<DataType>& getDataTypesToQuery() const { return typesToQuery; }

  // runtime check whether a specific data type is supported by sensor
  bool isDataTypeSupported (const DataType type) const { return dataTypeToNativeId (type) != -1; }

  // tare methods, not supported by default
  virtual bool setTareFull() { return false; }
  virtual bool setTareHeading() { return false; }
  virtual bool setTareTilt() { return false; }
  virtual bool resetTare() { return false; }
  virtual bool saveTare() { return false; }

  // reorientation methods
  virtual bool setReorientation (const Quaternion& newReorientation) { reorientation = newReorientation; return false; }
  virtual const Quaternion& getReorientation() const { return reorientation; }

  // sensor calibration methods, not supported by default
  virtual bool beginCalibration() { return false; }
  virtual bool endCalibration() { return false; }
  virtual bool saveCalibration() { return false; }
  virtual bool isCalibrating() const { return false; }

  // set sensor data rate
  virtual bool setDataRate (int rate) { return false; } // not supported by default

  // get current sensor/fusion reliability, 0.0..1.0
  virtual float getCurrentReliability() const { return 0.0f; } // not supported by default

  // get current sensor/fusion accuracy, radians, negative if invalid
  virtual float getCurrentAccuracy() const { return -1.0f; } // not supported by default

  bool isInitialised() const { return initialised; }

protected:
  // set/update data types to query, e.g., after setDataTypesToQuery()
  virtual bool updateDataTypesToQuery() = 0;

  // get array mapping data types to native sensors ids
  virtual const std::array<int, static_cast<int> (DataType::total_num)>& getDataTypeToNativeIdMap() const = 0;

  // get native id for data type
  int dataTypeToNativeId (const DataType type) const { return getDataTypeToNativeIdMap().at(static_cast<int> (type)); }

  // get data type for native id
  DataType nativeIdToDataType (const int id) const;
  
  // data types to query
  std::vector<DataType> typesToQuery;

  // type of last queried data
  DataType lastType;

  // initialised flag
  bool initialised;

  // reorientation quaternion
  Quaternion reorientation;

}; // class Imu

} // namespace Imag
