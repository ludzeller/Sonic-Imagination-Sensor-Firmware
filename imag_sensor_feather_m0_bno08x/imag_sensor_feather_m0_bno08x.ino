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
#include <EasyButton.h>

// should go into separate file
#define BUTTON_A  9
#define BUTTON_B  6
#define BUTTON_C  5

// battery status
#define VBATPIN A7 // == D9, so be aware of concurrency w/ button A

// member data
Imag::Imu_BNO08x imu;
Imag::Net_WINC150x net;
Adafruit_SSD1306 display { 128, 32, &Wire };   // oled display

EasyButton displayButton(BUTTON_B, 35, true, true);
EasyButton calibButton(BUTTON_C, 35, true, true);

// data types
static constexpr auto primaryDataType = Imag::Imu::DataType::rotation_arvr;

// reliability && accuracy smoothing buffers
static constexpr auto smoothLen = 100;
std::array<float, smoothLen> reliability;
std::array<float, smoothLen> accuracy;
auto smoothIndex = 0;
auto reliabilitySum = 0.0f;
auto accuracySum = 0.0f;

// display state
auto displayOn = true;

void beginCalibration()
{
  if (imu.isCalibrating())
    return;

  imu.beginCalibration();
  imu.printSensorsPerformingDynamicCalibration();
}


void endCalibration()
{
  if (! imu.isCalibrating())
    return;

  imu.endCalibration();
  imu.printSensorsPerformingDynamicCalibration();
}


void toggleDisplay()
{
  displayOn = ! displayOn;
  display.clearDisplay();
  display.display();
}


void setup()
{
  // prep
  pinMode (LED_BUILTIN, OUTPUT);

  // cleanup/separate!
//  pinMode(BUTTON_A, INPUT_PULLUP);
//  pinMode(BUTTON_B, INPUT_PULLUP);
//  pinMode(BUTTON_C, INPUT_PULLUP);

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
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(2);
  display.setCursor (0, 0);
  display.print("ImagSens");
  display.display();

  // smooth buffers: clean up!
  reliability.fill (0.0f);
  accuracy.fill (0.0f);

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
  if (! imu.setDataTypesToQuery ({ primaryDataType }))
  {
    DBGLN("failed to set custom data types to query");
  }

  // init network transport
  if (! net.init (Imag::Config::localIP))
  {
    DBGLN("Starting wifi failed");
    Imag::Debug::halt();
  }

  // init button
  calibButton.begin();
  calibButton.onPressed(endCalibration);
  calibButton.onPressedFor(2000, beginCalibration);

  displayButton.begin();
  displayButton.onPressed(toggleDisplay);
}


void loop()
{
  static auto displayRateTime = millis();
  static auto connMsgTime = millis();

  auto now = millis();

  // update/check current network status
  net.updateConnectionState();

  if (! net.isConnected() && now > connMsgTime)
  {
    // output message every 2s
    DBGLN("Waiting for wifi connection...");
    connMsgTime = now + 2000;
  }

  // eval buttons
  calibButton.read();
  displayButton.read();

//  // check for button press
//  if (digitalRead (BUTTON_A) == LOW)
//  {
//    imu.beginCalibration();
//    imu.printSensorsPerformingDynamicCalibration();
//  }
//  else if (digitalRead (BUTTON_B) == LOW)
//  {
//    imu.endCalibration();
//    imu.printSensorsPerformingDynamicCalibration();
//  }
//  else if (digitalRead (BUTTON_C) == LOW)
//  {
//    DBGLN("Setting tare of all axes");
//    imu.setTareFull();
//  }

  // read sensor data and send
  if (imu.available())
  {
    if (! imu.read())
    {
      DBGLN("failure reading sensor data");
      return;
    }

    // accumulate reliability and accuracy for smoothing

    if ((imu.isCalibrating() && imu.getLastDataType() == Imag::Imu::DataType::mag))
    {
      // imu.printCalibrationReliability();

      reliabilitySum -= reliability[smoothIndex];
      reliability[smoothIndex] = imu.getCurrentReliability();
      reliabilitySum += reliability[smoothIndex];

      smoothIndex = (smoothIndex + 1) % smoothLen;
    }

    else if (imu.getLastDataType() == primaryDataType)
    {
      reliabilitySum -= reliability[smoothIndex];
      reliability[smoothIndex] = imu.getCurrentReliability();
      reliabilitySum += reliability[smoothIndex];

      accuracySum -= accuracy[smoothIndex];
      accuracy[smoothIndex] = imu.getCurrentAccuracy();
      accuracySum += accuracy[smoothIndex];
      
      smoothIndex = (smoothIndex + 1) % smoothLen;
    }


    // time for display update?
    if (now > displayRateTime)
    {
      auto rel = reliabilitySum / smoothLen;
      auto acc =  constrain ((accuracySum / smoothLen) * 180.0 / PI, 0.0, 31.0);
      
      displayRateTime = now + 100; // next refresh

      DBG("smoothed reliability: "); DBGLN(rel);
      DBG("smoothed accuracy: "); DBGLN(acc);

      // measure battery voltage
      float battery = analogRead(VBATPIN);
      battery *= 2.0f;    // we divided by 2, so multiply back
      battery *= 3.3f;  // Multiply by 3.3V, our reference voltage
      battery /= 1024.0f; // convert to voltage
      DBG("Battery: "); DBGLN(battery);

      if (displayOn)
      {
        display.clearDisplay();
        display.setTextSize(1);
        display.setCursor (0, 0);
        display.println ("ImagSens 0.0.1");
        display.print("Bat: "); display.print(battery, 1); display.println("V");
        display.println();
        
        if (imu.isCalibrating())
        {
          display.println("Calibrating...");
          display.drawRect (88, 0, 16, 32, SSD1306_WHITE); // empty accuracy
        }
        else
        {
          display.println (net.isConnected() ? "Sending" : "Disconnected");
            
          // reported accuracy
          display.fillRect (88, 0, 16, int (acc) + 1, SSD1306_WHITE);
        }
  
        // reliability
        display.fillRect (112, 32 - int (rel * 31) - 1, 16, int (rel * 31) + 1, SSD1306_WHITE);
        display.display();
      }
    }
    

    // skip network sending part if disconnected or calibrating
    if (imu.isCalibrating() || ! net.isConnected())
      return;

    if (imu.getDataAsOsc (net.getOscObject()))
    {
      if (! net.sendOsc())
      {
        DBGLN("Sending osc message failed");
        return;
      }

    // DBGLN("Sensor data successfully sent");
    }
    else
    {
      DBGLN("Osc message construction failed: memory issue?");
    }
  }
  else
  {
    // limit querying rate
    delay (5);

    // DBGLN("Sensor has no more data for this query loop");
  }

}
