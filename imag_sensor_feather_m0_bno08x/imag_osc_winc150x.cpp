/* imag_osc_winc150x.cpp
 * 
 * imagination sensor firmware
 * osc interface using wifi ATWINC1500
 * 
 * 2021-2024 rumori
 */

#include "imag_osc_winc150x.h"

// redefine DBG output macros for this module only
#if ! IMAG_NET_DEBUG
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
#endif // #if ! IMAG_NET_DEBUG

namespace imag::osc
{
WINC150x::WINC150x (const std::array<byte, 4>& newLocalAddr, short newLocalPort)
    : osc (oscMsgBuffer, oscMsgMaxArgs),
      localAddr (IPAddress (newLocalAddr[0], newLocalAddr[1], newLocalAddr[2], newLocalAddr[3])),
      localPort (newLocalPort),
      state (WL_NO_SHIELD),
      readyToSend (false),
      initDelayEnd (0)
{
    // configure pins for Adafruit ATWINC1500 feather
    WiFi.setPins (8, 7, 4, 2);
}


bool WINC150x::init (const char* ssid, const char* key, uint8_t channel)
{
    // check for wifi interface presence
    if (! isShieldPresent())
    {
        DBGLN("WINC150x: wifi shield not present");
        return false;
    }

    WiFi.config (localAddr);

    if (state = WiFi.beginAP (ssid, key, channel) !=  WL_AP_LISTENING)
    {
        DBGLN("WINC150x: starting access point failed");
        return false;
    }

    // low power mode
    WiFi.lowPowerMode();

    // indicate we are waiting for connection
    digitalWrite (LED_BUILTIN, HIGH);

    return true;
}


void WINC150x::updateConnectionState()
{
    // check if state actually changed
    if (state == WiFi.status())
    {
        // are we waiting for the udp init delay?
        if (isConnected() && ! isReadyToSend() && isInitDelayOver())
        {
#if IMAG_NET_DEBUG
            byte remoteMac[6];
            WiFi.APClientMacAddress (remoteMac);
            DBG ("WINC150x: Device connected to AP, MAC: ");
            printMacAddr (remoteMac);
            DBGLN();
#endif // IMAG_NET_DEBUG

            udp.begin (localPort);
            readyToSend = true;
        }

        // return in any case if state did not change
        return;
    }

    state = WiFi.status();

#if IMAG_NET_DEBUG
    printWifiStatus();
#endif // IMAG_NET_DEBUG

    // are we newly connected?
    if (state == WL_AP_CONNECTED)
    {
        // wait until settle
        startInitDelay();
    
        // indicate we are connected
        digitalWrite (LED_BUILTIN, LOW);
    }
    else // not connected anymore
    {
        readyToSend = false;
        udp.stop();
        digitalWrite (LED_BUILTIN, HIGH);
        DBGLN("WINC150x: Device disconnected from AP");
    }
}


bool WINC150x::sendQuaternion (const char* oscAddress, const Quaternion& quat)
{
    auto res = true;
    
    res &= osc.init (oscAddress);
    res &= osc.addFloat (quat.x);
    res &= osc.addFloat (quat.y);
    res &= osc.addFloat (quat.z);
    res &= osc.addFloat (quat.w);

    if (! res)
    {
        DBGLN("WINC150x::sendQuaternion(): Error constructing OSC message");
        return false;
    }

    if (! (res = sendOsc()))
        DBGLN("WINC150x::sendQuaternion(): Sending osc message failed");

    return res;
}


bool WINC150x::sendOsc()
{
    if (! isReadyToSend())
    {
        DBGLN("WINC150x: cannot send osc: no peer connected");
        return false;
    }

    if (udp.beginPacket (targetAddr, targetPort) &&
        udp.write (osc.getMessageBuf(), osc.getMessageSize()) == osc.getMessageSize() &&
        udp.endPacket())
        return true;

    DBGLN("WINC150x: sending osc failed");
    return false;
}


void WINC150x::printWifiStatus() const
{
    // print the SSID of the network you're attached to:
    DBG("WINC150x: SSID: "); DBGLN(WiFi.SSID());

    // print your WiFi shield's IP address:
    DBG("WINC150x: IP Address: "); DBGLN(IPAddress (WiFi.localIP()));

    // print the received signal strength:
    DBG("WINC150x: signal strength (RSSI): "); DBG(WiFi.RSSI()); DBGLN(" dBm");
}


void WINC150x::printMacAddr (const byte mac[6]) const
{
    for (int i = 5; i >= 0; i--)
    {
        if (mac[i] < 16)
            DBG("0");

        DBGHEX(mac[i]);
    
        if (i > 0)
            DBG(":");
    }
}

} // namespace imag::osc
