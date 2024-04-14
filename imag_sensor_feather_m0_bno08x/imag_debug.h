/* imag_debug.h
 * 
 * imagination sensor firmware
 * debug helpers
 * 
 * 2021-05 rumori
 */

#pragma once

#include <Arduino.h>

#include "imag_config.h"

// debug print to serial
#if IMAG_DEBUG
#define DBG(STR)     Serial.print (F(STR))
#define DBGLN(STR)   Serial.println (F(STR))
#define DBGN(VAR)    Serial.print (VAR)
#define DBGNLN(VAR)  Serial.println (VAR)
#define DBGHEX(NUM)  Serial.print (NUM, HEX)
#else
#define DBG          ;
#define DBGLN        ;
#define DBGN         ;
#define DBGNLN       ;
#define DBGHEX       ;
#endif // IMAG_DEBUG

namespace imag
{
struct Debug
{
    // wait for serial console and init or timeout
    static bool init()
    {
#if ! IMAG_DEBUG
        return true;
#else
        // wait for serial to become ready
        for (int i = 0; i < config::serialTimeout * 5; ++i)
        {
            if (Serial)
            {
                digitalWrite (LED_BUILTIN, LOW);
                Serial.begin (config::serialBaudrate);
                return true;
            }
            delay (200);
            digitalWrite (LED_BUILTIN, ! digitalRead (LED_BUILTIN));
        }

        digitalWrite (LED_BUILTIN, LOW);

        return false;
#endif // IMAG_DEBUG
    }

    // halt machine and indicate by led flickering
    static void halt()
    {
        while (1)
        {
            digitalWrite (LED_BUILTIN, ! digitalRead (LED_BUILTIN));
            delay (50);
        }
    }

}; // struct Debug

} // namespace imag
