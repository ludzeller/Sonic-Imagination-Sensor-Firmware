/* imag_debug.h
 * 
 * imagination sensor firmware
 * debug helpers
 * 
 * 2021-05 rumori
 */

#ifndef IMAG_DEBUG_H_INCLUDED
#define IMAG_DEBUG_H_INCLUDED

#include <Arduino.h>

#include "imag_config.h"

// debug print to serial
#if IMAG_DEBUG
#define DBG(DATA)    Serial.print (DATA)
#define DBGLN(DATA)  Serial.println (DATA)
#define DBGHEX(DATA) Serial.print (DATA, HEX)
#else
#define DBG          ;
#define DBGLN        ;
#define DBGHEX       ;
#endif // IMAG_DEBUG

namespace Imag
{
class Debug
{
  public:
    // wait for serial console and init or timeout
    static bool init()
    {
#if IMAG_DEBUG
      // wait for serial to become ready
      for (int i = 0; i < Config::serialTimeout * 5; ++i)
      {
        if (Serial)
        {
          digitalWrite (LED_BUILTIN, LOW);
          Serial.begin (Config::serialBaudrate);
          return true;
        }
        delay (200);
        digitalWrite (LED_BUILTIN, ! digitalRead (LED_BUILTIN));
      }

      digitalWrite (LED_BUILTIN, LOW);
#endif // IMAG_DEBUG

      return false;
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

}; // class Debug

} // namespace Imag

#endif // #ifndef IMAG_DEBUG_H_INCLUDED
