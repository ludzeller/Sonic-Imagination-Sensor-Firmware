/* imag_battery.cpp
 * 
 * imagination sensor firmware
 * battery state measurement
 * 
 * 2021-2024 rumori
 */

#include "imag_battery.h"
#include "imag_debug.h"

// redefine DBG output macros for this module only
#if ! IMAG_BATTERY_DEBUG
#undef DBG
#define DBG       ;
#undef DBGLN
#define DBGLN     ;
#undef DBGN
#define DBGN      ;
#undef DBGNLN
#define DBGNLN    ;
#undef DBGHEX
#define DBGHEX    ;
#endif // #if ! IMAG_BATTERY_DEBUG

namespace imag
{

Battery::Battery (uint8_t batPin, bool shouldPullup)
    : pin (batPin),
      pullup (shouldPullup),
      voltage (0.0f),
      readTime (0)
{}


void Battery::update()
{
    const auto now = millis();

    if (now < readTime)
        return;

    readTime = now + readInterval;

    readNow();
}


void Battery::readNow()
{
    pinMode (pin, INPUT); // disable potential pullup
    voltage = analogRead (pin);

    if (pullup)
        pinMode (pin, INPUT_PULLUP); // re-enable pullup if requested

    // we assume a 0.5 voltage divider, 3.3V reference and 10bit measurement
    static constexpr auto multiplier = 2.0f * 3.3f / 1024.0f;

    voltage *= multiplier;

    DBG("Battery: read voltage "); DBGNLN(voltage);
}


uint8_t Battery::getPercentage() const
{
    // using an approximation from
    // https://stackoverflow.com/questions/56266857/how-do-i-convert-battery-voltage-into-battery-percentage-on-a-4-15v-li-ion-batte
    if (voltage > 4.0f)
        return map (constrain (voltage, 4.0f, 4.2f), 4.0f, 4.2f, 80, 99);
    else
        return map (constrain (voltage, 3.6f, 4.0f), 3.6f, 4.0f, 1, 80);
}

} // namespace imag
