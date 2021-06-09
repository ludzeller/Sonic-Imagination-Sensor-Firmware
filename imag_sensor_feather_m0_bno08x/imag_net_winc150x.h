/* imag_net_winc150x.h
 * 
 * imagination sensor firmware
 * wifi udp interface using ATWINC1500
 * 
 * 2021-05 rumori
 */

#pragma once

#include "imag_net.h"

#include <WiFi101.h>
#include <WiFiUdp.h>

namespace Imag
{
class Net_WINC150x : public Net
{
  public:
    // constructor
    Net_WINC150x();
  
    // Wifi class interface
    bool init (const std::array<byte, 4>& newLocalAddr) override;
    void updateConnectionState() override;
    bool isConnected() const override { return state == WL_AP_CONNECTED; }
    void setTargetAddr (const std::array<byte, 4>& addr) override { targetAddr = IPAddress (addr[0], addr[1], addr[2], addr[3]); }
    void setTargetPort (short port) override { targetPort = port; }
    bool sendOsc() override;

  private:
    void printWifiStatus() const;
    void printMacAddr (const byte mac[6]) const;
  
    // wifi udp object
    WiFiUDP udp;

    // local ip address
    IPAddress localAddr;

    // target ip address
    IPAddress targetAddr;

    // target port
    short targetPort;

    // connection status
    int state;

};
}
