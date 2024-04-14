/* imag_osc_winc150x.h
 * 
 * imagination sensor firmware
 * osc interface using wifi ATWINC1500
 * 
 * 2021-2024 rumori
 */

#pragma once

#include <WiFi101.h>
#include <WiFiUdp.h>
#include <LiteOSCParser.h>
#include <Arduino_Helpers.h>
#include <AH/Math/Quaternion.hpp>

#include <array>

#include "imag_debug.h"

namespace imag::osc
{
class WINC150x
{
public:
    using LiteOSCParser = qindesign::osc::LiteOSCParser;

    // osc message max dimensions
    static constexpr auto oscMsgBuffer  = 256;
    static constexpr auto oscMsgMaxArgs = 8;

    // init delay for settling wifi after connection
    static constexpr auto initDelay = 1000; // 1s

    // constructor
    WINC150x (const std::array<byte, 4>& localAddr = { 192, 168, 1, 1 }, short localPort = 9336);
  
    // init ap listening
    bool init (const char* ssid, const char* key, uint8_t channel);

    // update connection state, should be called periodically
    void updateConnectionState();

    // get shield presence state
    bool isShieldPresent() const { return WiFi.status() != WL_NO_SHIELD; }

    // get waiting for connection state
    bool isWaitingForConnection() const { return state == WL_AP_LISTENING; }

    // get connection state
    bool isConnected() const { return state == WL_AP_CONNECTED; }

    // get ready to send flag
    bool isReadyToSend() const { return readyToSend; }
    
    // set target ip address and port
    void setTarget (const std::array<byte, 4>& address, short port)
    {
        targetAddr = IPAddress (address[0], address[1], address[2], address[3]);
        targetPort = port;
    }

    // osc message sending methods
    bool sendQuaternion (const char* oscAddress, const Quaternion& quat);

private:
    // send current state of osc messaging member
    bool sendOsc();

    // init delay helpers
    void startInitDelay() { initDelayEnd = millis() + initDelay; }
    bool isInitDelayOver() const { return millis() > initDelayEnd; }
    
    // debug helpers
    void printWifiStatus() const;
    void printMacAddr (const byte mac[6]) const;

    // osc messaging object
    LiteOSCParser osc;

    // wifi udp object
    WiFiUDP udp;

    // local ip address
    IPAddress localAddr;

    // local port
    short localPort;

    // target ip address
    IPAddress targetAddr;

    // target port
    short targetPort;

    // connection status
    int state;

    // ready to send: udp initialised$
    bool readyToSend;

    // timestamp for init delay end
    uint32_t initDelayEnd;
};
} // namespace imag::osc
