/* imag_display_sh110x.h
 * 
 * imagination sensor firmware
 * sh110x 128x64 display interface
 * 
 * 2021-2024 rumori
 */

#pragma once

#include <Adafruit_SH110X.h>

#include "imag_display_content.h"

namespace imag::display
{

class SH1107
{
public:
    // display dimensions
    static constexpr auto displayWidth = 128;
    static constexpr auto displayHeight = 64;

    // display timings [ms]
    static constexpr auto displayRefresh = 250UL;
    static constexpr auto displayAutoOff = 60000UL;

    // battery low voltage (should possibly go into config)
    static constexpr auto batteryLowVoltage = 3.7f;

    // constructor
    SH1107 (TwoWire* twi);

    // initialise display connection
    bool init (uint8_t i2cAddr, bool reset = true);

    // select content page to display
    void setPage (Page pageToDisplay) { page = pageToDisplay; }

    // call periodically to check for display refresh/auto-off
    void update();

    // switch display on or off
    // returns whether the state was actually changed
    // restart auto off cycle in any case
    bool setEnabled (bool shouldBeOn);

    // query display enablement state
    bool isEnabled() const { return displayOn; }

    // get a content reference
    Content& getContent() { return content; }
    
    // restart auto-off timeout
    void resetAutoOff() { autoOffTime = millis() + displayAutoOff; }

private:
    // show splash screen
    void showSplash();

    // show main content page
    void showMain();
    
    // show calibration page
    void showCalibration();

    // print battery state
    void printBattery();

    // display object
    Adafruit_SH1107 display;

    // content member
    Content content;

    // page to show
    Page page;
    
    // display enablement flag
    bool displayOn;

    // refresh time
    uint32_t refreshTime;

    // auto off time
    uint32_t autoOffTime;
};

} // namespace imag::display
