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

  // constructor
  Imu();

  // check sensor presence and initialise 
  virtual bool init() = 0;

  // check if data is available
  virtual bool available() = 0;

  // query data from sensor if available and set type member
  virtual bool read() = 0;

  // set data types to query from sensor
  bool setDataTypesToQuery (const std::initializer_list<DataType>& dataTypes);

  // get data types currently queried by sensor
  const std::vector<DataType>& getDataTypesToQuery() const { return typesToQuery; }

  // runtime check whether a specific data type is supported by sensor
  bool isDataTypeSupported (const DataType type) const { return dataTypeToNativeId (type) != -1; }

  // get type of previously queried data
  DataType getLastDataType() const { return lastType; }

  // get previously queried data as osc message
  virtual bool getDataAsOsc (LiteOSCParser& osc) const = 0;

  // sensor calibration methods, not supported by default
  virtual bool beginCalibration() { return false; }
  virtual bool endCalibration() { return false; }
  virtual bool isCalibrating() const { return false; }

  // set sensor data rate
  virtual bool setDataRate (int rate) { return false; } // not supported by default

  // get current sensor/fusion reliability
  virtual float getCurrentReliability() const { return 0.0f; } // not supported by default


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

}; // class Imu

} // namespace Imag
