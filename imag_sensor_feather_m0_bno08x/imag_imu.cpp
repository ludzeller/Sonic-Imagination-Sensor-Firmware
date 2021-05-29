/* imag_imu.cpp
 * 
 * imagination sensor firmware
 * generic imu interface
 * 
 * 2021-05 rumori
 */

#include "imag_imu.h"

using namespace Imag;

// osc address to be used for each data type
const char* Imu::oscAddr[]
{
  OscAddress::rotation,  // ROTATION
  OscAddress::rotation,  // ROTATION_GEO
  OscAddress::rotation,  // ROTATION_GAME
  OscAddress::accel_lin  // ACCEL_LINEAR
};
