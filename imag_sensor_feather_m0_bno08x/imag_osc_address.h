/* imag_osc_address.h
 * 
 * imagination sensor firmware
 * imu osc address list
 * 
 * 2021-2024 rumori
 */

#pragma once

namespace imag::osc
{
struct Address
{
    static constexpr auto none       { "/invalid" };
    static constexpr auto rotation   { "/rot" };  // rotation as a quaternion: 4 floats [ i, j, k, r ]
};

} // namespace imag::osc
