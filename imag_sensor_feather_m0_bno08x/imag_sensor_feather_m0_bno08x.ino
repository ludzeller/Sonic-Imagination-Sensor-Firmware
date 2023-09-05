/* sensor_firmware_feather_m0

   imagination sensor firmware
   main

   2021-05 rumori
*/

#include "imag_config.h"
#include "imag_debug.h"
#include "imag_imu_bno08x.h"
#include "imag_net_winc150x.h"

#include <MIDIUSB.h>

#include <Adafruit_SSD1306.h>
#include <EasyButton.h>

// #include <ArduinoLowPower.h>

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

EasyButton displayButton(BUTTON_A, 35, true, true);
EasyButton tareButton(BUTTON_B, 35, true, true);
EasyButton calibButton(BUTTON_C, 35, true, true);

static constexpr auto displayAutoOff = 60000UL;
auto displayTimeOut = millis() + displayAutoOff;

// sensor data types
static constexpr auto primaryDataType = Imag::Imu::DataType::rotation_arvr;

// sensor mounting orientation/output conventions
static std::array<Quaternion, 2> sensorOrientations { {
  { 1.0f, 0.0f, 0.0f, 0.0f },             // normal mounting, no reorientation
  { 0.7071068f, 0.0f, 0.0f, -0.7071068f }  // normal mounting, no reorientation w/ iem mrhat compat rotation
  // { 0.0f, -1.0f, 0.0f, 0.0f },         // mounted upside down front
  // { 0.0f, 0.0f, -1.0f, 0.0f },         // mounted upside down back : imagination prototype mounting
  // { 0.0f, -0.7071f, 0.7071f, 0.0f }    // mounted upside down back w/ iem mrhat compat rotation
} };

// orientation mode indicators
static constexpr std::array<const char*, 2> sensorOrientationModes {
  "[std]", // imagination
  "[iem]"  // iem MrHat compatible
};

static_assert (sensorOrientations.size() == sensorOrientationModes.size());

// current orientation mode
int orientationMode = 0;

// custom north offset
bool customNorth = false;
Quaternion customNorthOffset;

// reliability && accuracy smoothing buffers
static constexpr auto smoothLen = 100;
std::array<float, smoothLen> reliability;
std::array<float, smoothLen> accuracy;
auto smoothIndex = 0;
auto reliabilitySum = 0.0f;
auto accuracySum = 0.0f;

// display state
auto displayStateOn = true;


void resetDisplayTimeout()
{
  displayTimeOut = millis() + displayAutoOff;  
}


void displayOff()
{
  displayStateOn = false;
  display.clearDisplay();
  display.display();
}


void displayOn()
{
  displayStateOn = true;
  resetDisplayTimeout();
}


void toggleDisplay()
{
  displayStateOn ? displayOff() : displayOn();
}


void beginOrCancelCalibration()
{
  displayOn();
  
  if (imu.isCalibrating())
    imu.endCalibration();
  else
    imu.beginCalibration();

  imu.printSensorsPerformingDynamicCalibration();

}


void switchModeOrEndCalibration()
{
  // reset display timeout
  displayOn();

  if (! imu.isCalibrating())
  {
    // rotate mode
    orientationMode = ++orientationMode % sensorOrientations.size();
    imu.setReorientation (sensorOrientations[orientationMode]);
    
    return;
  }
  
  // end calibration
  imu.endCalibration();
  imu.saveCalibration();
  imu.printSensorsPerformingDynamicCalibration();
}


void setTare()
{
  displayOn();

  if (Imag::Imu::isAnyRotationDataType (imu.getLastDataType()))
  {
    Quaternion current;

    imu.getLastData (current);

    // leave only z-rotation
    // TODO

    customNorthOffset = -current;
    customNorth = true;
  }
}


void resetTare()
{
  displayOn();
  
  customNorthOffset = Quaternion::identity();
  customNorth = false;
}


void setup()
{
  // led
  pinMode (LED_BUILTIN, OUTPUT);
  digitalWrite (LED_BUILTIN, LOW);

  // init display: cleanup/separate!
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C); // Address 0x3C for 128x32
  display.dim(true);
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setCursor (0, 0);
  display.setTextSize(2);
  display.println("ImagSens");
  display.setTextSize(1);
  display.println();

#if IMAG_DEBUG
  display.println("Waiting for serial...");
#else
  display.println("Initialising...");
#endif // IMAG_DEBUG

  display.display();

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

  // adapt to mounting orientation of sensor
  imu.setReorientation (sensorOrientations[orientationMode]); // initial orientation

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

  // init buttons
  displayButton.begin();
  displayButton.onPressed (toggleDisplay);

  tareButton.begin();
  tareButton.onPressed (setTare);
  tareButton.onPressedFor (2000, resetTare);

  calibButton.begin();
  calibButton.onPressed (switchModeOrEndCalibration);
  calibButton.onPressedFor (2000, beginOrCancelCalibration);

  // smooth buffers: clean up!
  reliability.fill (0.0f);
  accuracy.fill (0.0f);
}


void loop()
{
  auto now = millis();
  static auto displayRateTime = now;
  static auto connMsgTime = now;

  // flag for any received data
  auto dataReceived = false;

  // update/check current network status
  net.updateConnectionState();

  if (! net.isConnected() && now > connMsgTime)
  {
    // output message every 2s
    DBGLN("Waiting for wifi connection...");
    connMsgTime += 2000;
  }

  // eval buttons
  displayButton.read();
  tareButton.read();
  calibButton.read();

  // display timeout
  if (displayStateOn && now > displayTimeOut)
    displayOff();

  // read sensor data and send
  while (imu.available())
  {
    if (! imu.read())
    {
      DBGLN("failure reading sensor data");
      break;
    }

    dataReceived = true;

    // accumulate reliability and accuracy for smoothing
    if (imu.getLastDataType() == primaryDataType)
    {
      reliabilitySum -= reliability[smoothIndex];
      reliability[smoothIndex] = imu.getCurrentReliability();
      reliabilitySum += reliability[smoothIndex];

      accuracySum -= accuracy[smoothIndex];
      accuracy[smoothIndex] = imu.getCurrentAccuracy();
      accuracySum += accuracy[smoothIndex];

      smoothIndex = (smoothIndex + 1) % smoothLen;
    }
    else if ((imu.isCalibrating() && imu.getLastDataType() == Imag::Imu::DataType::mag))
    {
      reliabilitySum -= reliability[smoothIndex];
      reliability[smoothIndex] = imu.getCurrentReliability();
      reliabilitySum += reliability[smoothIndex];

      smoothIndex = (smoothIndex + 1) % smoothLen;
    }

    // send data
    if (Imag::Imu::isAnyRotationDataType (imu.getLastDataType()))
    {
      // get data
      Quaternion rot;
      auto success = true;
      
      imu.getLastData (rot);

      // custom north
      rot = customNorthOffset + rot;

      // send midi
      std::array<float, 4> rotAsFloats { rot.w, rot.x, rot.y, rot.z };
      uint8_t msg[4];

      msg[0] = 0x0b;
      msg[1] = 0xb0 | 0x01; // midi channel 1

      // send each part of quaternion as 14-bit midi CC
      for (auto i = 0; i < 4; ++i)
      {
        auto value = uint16_t ((rotAsFloats[i] + 1.0f) * 8192.0f); // 0..16384, 14bit

        // coarse
        msg[2] = i + 16; // cc coarse
        msg[3] = value >> 7 & 0x7f;
        success &= MidiUSB.write (msg, 4) == 4;

        // fine
        msg[2] = i + 48; // cc fine
        msg[3] = value & 0x7f;
        success &= MidiUSB.write (msg, 4) == 4;
      }

      if (! success)
        DBGLN("Error sending MIDI data");

      // skip network sending part if disconnected or calibrating
      if (imu.isCalibrating() || ! net.isConnected())
        continue;

      // send osc
      auto& osc = net.getOscObject();
      success = true;
    
      success &= osc.init (Imag::OscAddress::rotation);
      success &= osc.addFloat (rot.x);
      success &= osc.addFloat (rot.y);
      success &= osc.addFloat (rot.z);
      success &= osc.addFloat (rot.w);

      if (! success)
      {
        DBGLN("Error constructing OSC message");
      }
      else if (! net.sendOsc())
      {
        DBGLN("Sending osc message failed");
      }
      else
      {
        ; // DBGLN("Sensor data successfully sent");
      }
    }
    else
    { 
      DBGLN("Cannot send non-rotation data type in current version");
    }
  }
  // DBGLN("Sensor has no more data for this query loop");

  // time for display update?
  if (displayStateOn && now > displayRateTime)
  {
    // measure battery voltage
    pinMode(BUTTON_A, INPUT); // disable pullup for button, read voltage
    float battery = analogRead(VBATPIN);
    pinMode(BUTTON_A, INPUT_PULLUP); // re-enable pullup for button
    battery *= 2.0f;    // we divided by 2, so multiply back
    battery *= 3.3f;  // Multiply by 3.3V, our reference voltage
    battery /= 1024.0f; // convert to voltage
    DBG("Battery: "); DBGLN(battery);

    // calculate effective reliability/accuracy to display
    auto rel = reliabilitySum / smoothLen;
    auto acc =  constrain ((accuracySum / smoothLen) * 180.0 / PI, 0.0, 23.0);

    DBG("smoothed reliability: "); DBGLN(rel);
    DBG("smoothed accuracy: "); DBGLN(acc);

    // display
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor (0, 0);
    display.print ("ImagSens "); display.print (sensorOrientationModes[orientationMode]); display.println (" ac  re");
    display.print ("Battery: "); display.print (battery, 1); display.println ("V");
    display.println (customNorth ? "[Custom North]" : "");

    if (imu.isCalibrating())
    {
      display.println("Calibrating...");
      display.drawRect (88, 8, 16, 24, SSD1306_WHITE); // empty accuracy

      // reset display timeout while calibrating
      resetDisplayTimeout();
    }
    else
    {
      display.println (net.isConnected() ? "Sending" : "Disconnected");

      // reported accuracy
      display.fillRect (88, 8, 16, int (acc) + 1, SSD1306_WHITE);
    }

    // reliability
    display.fillRect (112, 32 - int (rel * 23) - 1, 16, int (rel * 23) + 1, SSD1306_WHITE);
    display.display();

    // next display refresh
    displayRateTime += 200;
  }

  // limit querying rate
  auto elapsed = millis() - now;

  if (dataReceived)
    DBG("data received. ");
  DBG("loop took [ms]: "); DBGLN(elapsed);

  if (dataReceived && elapsed < 10)
  {
    // sleep until next tick of our sensor rate (100 Hz for now)
    // waking up seems to take way to long - not usable atm.
    //LowPower.idle (10 - elapsed);

    delay (10 - elapsed);
  }
}
