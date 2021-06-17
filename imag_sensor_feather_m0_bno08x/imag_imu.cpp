/* imag_imu.cpp
 * 
 * imagination sensor firmware
 * generic imu interface
 * 
 * 2021-05 rumori
 */

#include "imag_imu.h"

using namespace Imag;

const std::array<const char* const, static_cast<int> (Imu::DataType::total_num)> Imu::oscAddr
{
  OscAddress::none,     // accel
  OscAddress::none,     // gyro
  OscAddress::none,     // mag
  OscAddress::rotation, // rotation
  OscAddress::rotation, // rotation_game
  OscAddress::rotation, // rotation_geo
  OscAddress::rotation, // rotation_arvr
  OscAddress::rotation, // rotation_game_arvr
  OscAddress::none,     // tap_detect
  OscAddress::none,     // step_detect
  OscAddress::none,     // step_count
  OscAddress::none,     // significant_motion
  OscAddress::none,     // stability_detect
  OscAddress::none,     // stability_class
  OscAddress::none,     // activity_class
  OscAddress::none      // shake_detect
};

Imu::Imu()
  : typesToQuery { DataType::rotation },
    lastType (DataType::none),
    initialised (false)
{
}


bool Imu::setDataTypesToQuery (const std::initializer_list<DataType>& dataTypes)
{
  auto res = true;

  typesToQuery.clear();
  
  for (auto type : dataTypes)
  {
    if (isDataTypeSupported (type))
      typesToQuery.push_back (type);
    else
  	  res = false;
  }

  // try to update supported types even if also unsupported were requested
  return updateDataTypesToQuery() && res;
}


Imu::DataType Imu::nativeIdToDataType (const int id) const
{
  const auto& map = getDataTypeToNativeIdMap();

  for (auto i = 0; i < map.size(); ++i)
    if (map[i] == id)
      return static_cast<DataType> (i);

  return DataType::none;
}
