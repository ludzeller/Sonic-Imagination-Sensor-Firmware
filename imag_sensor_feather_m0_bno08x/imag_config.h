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
#define IMAG_DISPLAY_DEBUG 0 // display low-level debug
#define IMAG_BATTERY_DEBUG 0 // battery low-level debug

namespace imag::config
{
// sensor-individual configuration
static constexpr auto sensorIndex = 7;

// exhibition mode?
/* Setting this to true will change the button behaviour to prevent
   unintended configuration changes.
 */
static constexpr auto guidedAccess = false;

// version information
static constexpr auto versionMajor = 0;
static constexpr auto versionMinor = 5;
static constexpr auto versionSub   = 3;

// seconds to wait at bootup for serial debug console
static constexpr auto serialTimeout = 5;
  
// serial baudrate
static constexpr auto serialBaudrate = 115200;

// display configuration
struct Display
{
    static constexpr uint8_t i2cAddr = 0x3c;
};

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
    static constexpr auto ssid = "ImagSens";
    static constexpr auto key = "atmospheres";
    static constexpr uint8_t channel = sensorIndex;
};

// bno08x hardware configuration
struct BNO08x
{
    static constexpr uint8_t i2cAddr = 0x4b; // adafruit breakout: 0x4a, slimevr: 0x4b
    static constexpr uint8_t intPin = 11;
    static constexpr uint8_t resetPin = 12;
};

// button configuration
struct Button
{
    static constexpr uint8_t pinA = 9;
    static constexpr uint8_t pinB = 6;
    static constexpr uint8_t pinC = 5;
    static constexpr uint32_t debounce = 35;
};

// battery measurement configuration
struct Battery
{
    static constexpr uint8_t pin = A7; // == D9, so be aware of concurrency w/ button A
};

} // namespace imag::config
