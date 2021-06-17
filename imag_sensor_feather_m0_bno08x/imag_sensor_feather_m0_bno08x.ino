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

#include <Adafruit_SSD1306.h>

// should go into separate file
#define BUTTON_A  9
#define BUTTON_B  6
#define BUTTON_C  5

// battery status
#define VBATPIN A6

Adafruit_SSD1306 display { 128, 32, &Wire };   // oled display

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

  // init display: cleanup/separate!
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C); // Address 0x3C for 128x32
  display.dim(true);
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // init sensor
  if (! imu.init())
  {
    DBGLN("Sensor init failed");
    Imag::Debug::halt();
  }

  // adapt to mounting orientation of sensor
  //imu.setReorientation (0.0, 0.0, 0.0, 1.0);  // normal, no reorientation
  //imu.setReorientation (-1.0, 0.0, 0.0, 0.0); // upside down front
  imu.setReorientation (0.0, -1.0, 0.0, 0.0); // upside down back
  
	imu.printSensorsPerformingDynamicCalibration();

  // set data types we are interested in
  imu.setDataTypesToQuery ({ Imag::Imu::DataType::rotation });
  //imu.setDataTypesToQuery ({ Imag::Imu::DataType::rotation_arvr });

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

  if (! net.isConnected())
  {
    DBGLN("Waiting for wifi connection...");
    delay (2000);
    return;
  }

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
  else if (digitalRead (BUTTON_C) == LOW)
  {
    DBGLN("Setting tare of all axes");
    imu.setTareFull();
  }

  // read sensor data and send
  while (imu.available())
  {
    if (! imu.read())
    {
      DBGLN("failure reading sensor data");
      continue;
    }
    auto acc = imu.getCurrentAccuracy();
    
    DBGLN(acc);

		if (imu.isCalibrating() && imu.getLastDataType() == Imag::Imu::DataType::mag)
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
	DBGLN("Sensor has no more data for this query loop");
  
  // limit querying rate
  delay (5);
}
