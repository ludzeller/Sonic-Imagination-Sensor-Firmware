/* imag_imu.h
 * 
 * imagination sensor firmware
 * generic imu interface
 * 
 * 2021-05 rumori
 */

#ifndef IMAG_IMU_H_INCLUDED
#define IMAG_IMU_H_INCLUDED

#include <LiteOSCParser.h>
using LiteOSCParser = qindesign::osc::LiteOSCParser;

#include "imag_debug.h"

namespace Imag
{
namespace OscAddress
{
  static constexpr auto rotation   { "/rotation" };  // rotation as a quaternion: 4 floats [ r, i, j, k ] 
  static constexpr auto accel_lin  { "/accel_lin" }; // linear acceleration: 3 floats [ x, y, z ]  
}

class Imu
{
  public:
    // data types that can be queried from sensor
    enum DataType
    {
      NONE         = -1,
      ROTATION     = 0,
      ROTATION_GEO,
      ROTATION_GAME,
      ACCEL_LINEAR,
      
    };

    // osc address to be used for each data type
    static const char* oscAddr[];

    // constructor
    Imu()
      : type (NONE)
    { }

    // check sensor presence and initialise 
    virtual bool init() = 0;

    // query data from sensor if available and set type member
    virtual bool queryData() = 0;

    // get previously queried data as osc message
    virtual bool getDataAsOsc (LiteOSCParser& osc) const = 0;

    // get type of previously queried data
    DataType getDataType() const { return type; }

  protected:
    // type of last queried data
    DataType type;

}; // class Imu

} // namespace Imag

#endif // #ifndef IMAG_IMU_H_INCLUDED
