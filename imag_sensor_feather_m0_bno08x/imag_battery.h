/* imag_battery.h
 * 
 * imagination sensor firmware
 * battery state measurement
 * 
 * 2021-2024 rumori
 */

#pragma once

#include "imag_config.h"

namespace imag
{

class Battery
{
public:
    // battery measurement refresh rate
    static constexpr auto readInterval = 10000UL;

    // constructor
    // if shoudPullup is set to true, batPin will be set back to INPUT_PULLUP after each analogRead()
    // this is handy if batPin is used, e.g., as a button pin at the same time
    Battery (uint8_t batPin, bool shouldPullup = false);

    // call periodically to check for read interval
    void update();

    // measure the voltage right now, query with getVoltage()
    void readNow();

    // return last measured voltage
    float getVoltage() const { return voltage; }

    // return an estimate of the capacity left
    uint8_t getPercentage() const;

private:
    // measurement pin
    uint8_t pin;

    // concurrent read flag
    bool pullup;

    // last measured result
    float voltage;

    // measurement time
    uint32_t readTime;
};

} // namespace imag
