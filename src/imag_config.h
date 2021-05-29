/* imag_config.h
 * 
 * imagination sensor firmware
 * global configuration
 * 
 * 2021-05 rumori
 */

#ifndef IMAG_CONFIG_H_INCLUDED
#define IMAG_CONFIG_H_INCLUDED

#include <Arduino.h>
#include <array>

// enable debugging output?
#define IMAG_DEBUG      1 // general debug
#define IMAG_IMU_DEBUG  0 // Imu low-level debug
#define IMAG_WIFI_DEBUG 0 // Wifi low-level debug

namespace Imag
{
namespace Config
{
  // version information
  static constexpr auto versionMajor = 0;
  static constexpr auto versionMinor = 0;
  static constexpr auto versionSub   = 1;
  
  // seconds to wait for serial debug console after boot
  static constexpr auto serialTimeout = 5;
  // serial baudrate
  static constexpr auto serialBaudrate = 115200;

  // default local ip address
  static constexpr std::array<byte, 4> localIP { 192, 168, 1, 1 };

  // default local (listening) port
  static constexpr auto localPort = 9336;
    
  // default target ip address
  static constexpr std::array<byte, 4> remoteIP { 192, 168, 1, 100 };

  // default target port
  static constexpr auto remotePort = 9336;

  // default wifi ap ssid and key
  static constexpr auto wifiSSID = "imagination";
  static constexpr auto wifiPASS = "atmospheres";
}
}


#endif // #ifndef IMAG_CONFIG_H_INCLUDED
