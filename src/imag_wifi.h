/* imag_wifi.h
 * 
 * imagination sensor firmware
 * generic wifi udp interface 
 * 
 * 2021-05 rumori
 */

#ifndef IMAG_WIFI_H_INCLUDED
#define IMAG_WIFI_H_INCLUDED

#include <array>

#include "imag_config.h"
#include "imag_debug.h"

#include <LiteOSCParser.h>

using LiteOSCParser = qindesign::osc::LiteOSCParser;

namespace Imag
{
class Wifi
{
  public:
    // osc message max dimensions
    static constexpr auto oscMsgBuffer  = 256;
    static constexpr auto oscMsgMaxArgs = 8;

    // constructor
    Wifi()
      : osc (oscMsgBuffer, oscMsgMaxArgs)
    { }

    // init ap listening
    virtual bool initAp (const std::array<byte, 4>& newLocalAddr = { 192, 168, 1, 1 }) = 0;

    // update connection state, should be called periodically
    virtual void updateConnectionState() = 0;    

    // get connection state
    virtual bool isConnected() const = 0;

    // set target ip address and port
    virtual void setTargetAddr (const std::array<byte, 4>& addr) = 0;
    virtual void setTargetPort (short port) = 0;
    
    // send osc message member
    virtual bool sendOsc() = 0;

    // return a reference to the osc object
    LiteOSCParser& getOsc() { return osc; }

  protected:
    LiteOSCParser osc;

};
}

#endif // #ifndef IMAG_WIFI_H_INCLUDED
