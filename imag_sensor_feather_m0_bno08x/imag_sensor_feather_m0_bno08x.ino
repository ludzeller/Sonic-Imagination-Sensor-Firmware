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
#include "imag_net_winc150x.h"

// should go into separate file
#define BUTTON_A  9
#define BUTTON_B  6
#define BUTTON_C  5
  
// member data
Imag::Imu_BNO08x imu;
Imag::Net_WINC150x net;

void setup()
{
  // prep
  pinMode (LED_BUILTIN, OUTPUT);

  // cleanup/separate!
  pinMode(BUTTON_A, INPUT_PULLUP);
  pinMode(BUTTON_B, INPUT_PULLUP);
  pinMode(BUTTON_C, INPUT_PULLUP);

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
	else
	{
		imu.printSensorsPerformingDynamicCalibration();
	}

  // init network transport
  if (! net.init (Imag::Config::localIP))
  {
    DBGLN("Starting wifi failed");
    Imag::Debug::halt();
  }
}


void loop()
{
  // update/check current network status
  net.updateConnectionState();

//  if (! net.isConnected())
//  {
//    DBGLN("Waiting for wifi connection...");
//    delay (2000);
//    return;
//  }

	// check for button press
	if (digitalRead (BUTTON_A) == LOW)
	{
		imu.beginCalibration();
		imu.printSensorsPerformingDynamicCalibration();
	}
	else if (digitalRead (BUTTON_B) == LOW)
	{
		imu.endCalibration();
		imu.printSensorsPerformingDynamicCalibration();
	}
	
  // read sensor data and send
  while (imu.available())
  {
		imu.read();

		//		if (imu.isCalibrating() && imu.getLastDataType() == Imag::Imu::DataType::mag)
		imu.printCalibrationReliability();

		// skip network sending part if disconnected or calibrating
		if (imu.isCalibrating() || ! net.isConnected())
			continue;
		
		if (imu.getDataAsOsc (net.getOscObject()))
			{
				if (! net.sendOsc())
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
	//  DBGLN("Sensor has no more data for this query loop");
  
  // limit querying rate
  delay (5);
}
