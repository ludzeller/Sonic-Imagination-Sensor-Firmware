/* imag_net_winc150x.cpp
 * 
 * imagination sensor firmware
 * wifi udp interface using ATWINC1500
 * 
 * 2021-05 rumori
 */

#include "imag_net_winc150x.h"

#if ! IMAG_NET_DEBUG
#define DBG          ;
#define DBGLN        ;
#define DBGHEX       ;
#endif // #if ! IMAG_NET_DEBUG

using namespace Imag;

Net_WINC150x::Net_WINC150x()
  : state (WL_IDLE_STATUS),
    readyToSend (false),
    udpInitTime (millis()) // just in case
{
  // configure pins for Adafruit ATWINC1500 feather
  WiFi.setPins (8, 7, 4, 2);

  setTargetAddr (Config::remoteIP);
  setTargetPort (Config::remotePort);
}


bool Net_WINC150x::init (const std::array<byte, 4>& newLocalAddr)
{
  // check for wifi interface presence
  if (state = WiFi.status() == WL_NO_SHIELD)
  {
    DBGLN("Net_WINC150x: wifi shield not present");
    return false;
  }

  // assign and set local address
  localAddr = IPAddress (newLocalAddr[0], newLocalAddr[1], newLocalAddr[2], newLocalAddr[3]);
  WiFi.config (localAddr);

  if (state = WiFi.beginAP (Config::wifiSSID, Config::wifiPASS) !=  WL_AP_LISTENING)
  {
    DBGLN("Net_WINC150x: starting access point failed");
    return false;
  }

  // low power mode
  WiFi.lowPowerMode();

  // indicate we are waiting for connection
  digitalWrite (LED_BUILTIN, HIGH);

  return true;
}


void Net_WINC150x::updateConnectionState()
{
  // check if state actually changed
  if (state == WiFi.status())
  {
    // are we waiting for the udp init delay?
    if (state == WL_AP_CONNECTED && ! readyToSend && millis() > udpInitTime)
    {
#if IMAG_NET_DEBUG
      byte remoteMac[6];
      WiFi.APClientMacAddress (remoteMac);
      DBG ("Net_WINC150x: Device connected to AP, MAC: ");
      printMacAddr (remoteMac);
      DBGLN();
#endif // IMAG_NET_DEBUG

      udp.begin (Config::localPort);
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
    // set udp init time
    udpInitTime = millis() + 1000; // 1s from now
    //delay (1000);
    //udp.begin (Config::localPort);
    
    // indicate we are connected
    digitalWrite (LED_BUILTIN, LOW);
  }
  else // not connected anymore
  {
    readyToSend = false;
    udp.stop();
    digitalWrite (LED_BUILTIN, HIGH);
    DBGLN("Net_WINC150x: Device disconnected from AP");
  }
}


bool Net_WINC150x::sendOsc()
{
  if (! isConnected())
  {
    DBGLN("Net_WINC150x: cannot send osc: no peer connected");
    return false;
  }

  if (udp.beginPacket (targetAddr, targetPort) &&
      udp.write (osc.getMessageBuf(), osc.getMessageSize()) == osc.getMessageSize() &&
      udp.endPacket())
    return true;

   if (! udp.beginPacket (targetAddr, targetPort))
    DBGLN("error beginpacket");
   if (! udp.write (osc.getMessageBuf(), osc.getMessageSize()) == osc.getMessageSize())
    DBGLN("error write");
   if (! udp.endPacket())
   DBGLN("error endpacket");
   
    return true;


  DBGLN("Net_WINC150x: sending osc failed");
  return false;
}


void Net_WINC150x::printWifiStatus() const
{
  // print the SSID of the network you're attached to:
  DBG("Net_WINC150x: SSID: "); DBGLN(Config::wifiSSID);

  // print your WiFi shield's IP address:
  DBG("Net_WINC150x: IP Address: "); DBGLN(IPAddress (WiFi.localIP()));

  // print the received signal strength:
  DBG("Net_WINC150x: signal strength (RSSI): "); DBG(WiFi.RSSI()); DBGLN(" dBm");
}


void Net_WINC150x::printMacAddr (const byte mac[6]) const
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
