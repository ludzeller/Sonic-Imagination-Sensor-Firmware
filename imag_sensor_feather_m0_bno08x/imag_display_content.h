/* imag_display_content.h
 * 
 * imagination sensor firmware
 * display messages and content container
 * 
 * 2021-2024 rumori
 */

#pragma once

#include <array>

#include <Arduino_Helpers.h>
#include <AH/Math/Quaternion.hpp>

namespace imag::display
{
// content page selector
enum class Page
{
    none = 0x0,
    splash,
    main,
    calibration
};

// display message strings
struct Message
{
    // generic
    static constexpr auto dot = ".";
    static constexpr auto asterisk = "*";
    static constexpr auto underscore = "_";

    // splash screen
    struct Splash
    {
        static constexpr auto title = "ImagSens";
        static constexpr auto versionLabel = "version ";
        static constexpr auto debugSerialWait = "Waiting for serial...";
        static constexpr auto initialising = "Initialising...";
    };

    // main
    struct Main
    {
        // button labels
        static constexpr std::array<const char*, 3> buttons {
            "Config", // button C
            "North",  // button B
            "Screen"  // button A
        };

        // orientation mode indicators
        static constexpr std::array<const char*, 2> orientationConfig {
            "[imag]", // imagination
            "[iem]"  // iem MrHat compatible
        };

        // north indicators
        static constexpr auto magneticNorth = "[magn]";
        static constexpr auto customNorth = "[cstm]";

        // connection/sender labels
        static constexpr auto senderOsc = "OSC";
        static constexpr auto senderMidi = "MIDI";

        // reliability/accuracy indicators
        static constexpr auto reliability = "Acc";
        static constexpr auto accuracy = "Err";
    };

    struct Battery
    {
        // battery label
        static constexpr auto label = "Bat ";
        static constexpr auto low = "Bat Low";
        static constexpr auto lowLong = "BATTERY LOW";
    };

    struct Calibration
    {
        // label
        static constexpr auto label = "Calibrating...";
    
        // button labels
        static constexpr std::array<const char*, 3> buttons {
            "Store", // button C
            "Cancel",  // button B
            "Clear"  // button A
        };

        // calibration reliability indicator
        static constexpr auto reliability = "Accuracy";
    };
};


// container for data content to display
struct Content
{
    const char* version;

    const char* ssid;
    
    uint8_t orientationConfig = 0;

    bool customNorth = false;
    
    bool senderOsc = false;
    bool senderMidi = false;
    
    float reliability = 0.0f;
    float accuracy = 0.0f;

    Quaternion rotation;

    float batteryVoltage = 0.0f;
    uint8_t batteryPercentage = 0;
};

} // namespace imag::display
