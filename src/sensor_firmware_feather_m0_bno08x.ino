/* sensor_firmware_feather_m0
 * 
 * imagination sensor firmware
 * main
 * 
 * 2021-05 rumori
 */

#include "imag_config.h"
#include "imag_debug.h"
#include "imag_imu_bno08x.h"
#include "imag_wifi_winc150x.h"

// member data
Imag::Imu_BNO08x imu;
Imag::Wifi_WINC150x wifi;

void setup()
{
  // prep
  pinMode (LED_BUILTIN, OUTPUT);
  
  // init debug serial console in case debugging is enabled
  Imag::Debug::init();
  DBG("Imagination sensor version ");
  DBG(Imag::Config::versionMajor); DBG(".");
  DBG(Imag::Config::versionMinor); DBG(".");
  DBGLN(Imag::Config::versionSub);

  // init sensor
  if (! imu.init())
  {
    DBGLN("Sensor init failed");
    Imag::Debug::halt();
  }

  // init wifi ap
  if (! wifi.initAp (Imag::Config::localIP))
  {
    DBGLN("Starting wifi AP failed");
    Imag::Debug::halt();
  }
}

void loop()
{
  // update/check current wifi status
  wifi.updateConnectionState();

  if (! wifi.isConnected())
  {
    DBGLN("Waiting for wifi connection...");
    delay (2000);
    return;
  }

  // read sensor data and send
  while (imu.queryData())
  {
    if (imu.getDataAsOsc (wifi.getOsc()))
    {
      if (! wifi.sendOsc())
      {
        DBGLN("Sending osc message failed");
        break;
      }

      DBGLN("Sensor data successfully sent");
    }
    else
    {
      DBGLN("Osc message construction failed: memory issue?");
    }
  }
  DBGLN("Sensor has no more data for this query loop");
  
  // limit querying rate
  delay (5);
}
