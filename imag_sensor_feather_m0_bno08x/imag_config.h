/* imag_config.h
 * 
 * imagination sensor firmware
 * global configuration
 * 
 * 2021-2024 rumori
 */

#pragma once

#include <Arduino.h>
#include <array>

// enable debugging output?
#define IMAG_DEBUG     0 // general debug
#define IMAG_IMU_DEBUG 0 // Imu low-level debug
#define IMAG_NET_DEBUG 0 // Wifi low-level debug

namespace imag::config
{
// version information
static constexpr auto versionMajor = 0;
static constexpr auto versionMinor = 5;
static constexpr auto versionSub   = 0;
  
// seconds to wait at bootup for serial debug console
static constexpr auto serialTimeout = 5;
  
// serial baudrate
static constexpr auto serialBaudrate = 115200;

// network configuration
struct Net
{
    static constexpr std::array<byte, 4> localIP { 192, 168, 1, 1 }; // local ip address
    static constexpr short localPort = 9336; // local (listening) port

    static constexpr std::array<byte, 4> remoteIP { 192, 168, 1, 100 }; // target ip address
    static constexpr auto remotePort = 9336; // target port
};

// wifi configuration
struct WiFi
{
    static constexpr auto ssid = "imagination";
    static constexpr auto key = "atmospheres";
    static constexpr uint8_t channel = 1;
};

// bno08x hardware configuration
struct BNO08x
{
static constexpr uint8_t i2cAddr = 0x4b; // adafruit breakout: 0x4a, slimevr: 0x4b
static constexpr uint8_t intPin = 11;
static constexpr uint8_t resetPin = 12;
};

} // namespace imag::config
