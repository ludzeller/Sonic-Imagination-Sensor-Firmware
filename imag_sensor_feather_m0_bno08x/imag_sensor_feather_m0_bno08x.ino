/* sensor_firmware_feather_m0

   imagination sensor firmware
   main

   2021-2024 rumori
*/

#include "imag_config.h"
#include "imag_debug.h"
#include "imag_osc_address.h"
#include "imag_smoother.h"

#include "imag_imu_bno08x.h"
#include "imag_osc_winc150x.h"

#include <MIDIUSB.h>

#include <Adafruit_SSD1306.h> // 128x32 oled display
#include <Adafruit_SH110X.h>  // 128x64 oled display
#include <EasyButton.h>

// #include <ArduinoLowPower.h>

// should go into separate file
#define BUTTON_A  9
#define BUTTON_B  6
#define BUTTON_C  5

// battery status
#define VBATPIN A7 // == D9, so be aware of concurrency w/ button A

// member data
imag::imu::BNO08x imu { imag::config::BNO08x::resetPin, imag::config::BNO08x::intPin };


const auto localAddr = imag::config::Net::localIP;
imag::osc::WINC150x net { localAddr, imag::config::Net::localPort };

//Adafruit_SSD1306 display { 128, 32, &Wire };  // oled display 128x32
Adafruit_SH1107 display { 64, 128, &Wire };   // oled display 128x64

EasyButton displayButton(BUTTON_A, 35, true, true);
EasyButton tareButton(BUTTON_B, 35, true, true);
EasyButton calibButton(BUTTON_C, 35, true, true);

static constexpr auto displayAutoOff = 60000UL;
auto displayTimeOut = millis() + displayAutoOff;

// sensor data types
static constexpr auto primaryDataType = imag::imu::DataType::rotationArvr;

// sensor mounting orientation/output conventions
static std::array<Quaternion, 2> sensorOrientations {
    {
        { 1.0f, 0.0f, 0.0f, 0.0f },             // normal mounting, no reorientation
        { 0.7071068f, 0.0f, 0.0f, -0.7071068f }  // normal mounting, no reorientation w/ iem mrhat compat rotation
        // { 0.0f, -1.0f, 0.0f, 0.0f },         // mounted upside down front
        // { 0.0f, 0.0f, -1.0f, 0.0f },         // mounted upside down back : imagination prototype mounting
        // { 0.0f, -0.7071f, 0.7071f, 0.0f }    // mounted upside down back w/ iem mrhat compat rotation
    }
};

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
Quaternion customNorthOffset = Quaternion::identity();

// reliability && accuracy smoothers
static constexpr auto smoothLen = 100;
static imag::Smoother<float, smoothLen> reliability, accuracy;

// display state
auto displayStateOn = true;

// int flag
auto intHigh = true;


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

    if (imag::imu::isAnyRotationDataType (imu.getLastDataType()))
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
    // delay (500); // give the baby some time to settle

    //Wire.setClock(200000L);   // I2C speed, 400 kHz is a little fast for Arduino's pullups

    // led
    pinMode (LED_BUILTIN, OUTPUT);
    digitalWrite (LED_BUILTIN, LOW);

    // init display: cleanup/separate!
    // 128x32
    // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
    /*
      display.begin (SSD1306_SWITCHCAPVCC, 0x3C);
      delay (100);
      display.dim (true);
      display.setTextColor (SSD1306_WHITE);
    */  
  
    // 128x64
    display.begin (0x3C, true);
    display.setRotation (1);
    display.setTextColor (SH110X_WHITE);

    display.clearDisplay();
    display.setCursor (0, 0);
    display.setTextSize (2);
    display.println ("ImagSens");
    display.setTextSize (1);
    display.println();

#if IMAG_DEBUG
    display.println ("Waiting for serial...");
#else
    display.println ("Initialising...");
#endif // IMAG_DEBUG

    display.display();

    // init debug serial console in case debugging is enabled
    imag::Debug::init();
    DBG("Imagination sensor version ");
    DBGN(imag::config::versionMajor); DBG(".");
    DBGN(imag::config::versionMinor); DBG(".");
    DBGNLN(imag::config::versionSub);

    // init sensor
    if (! imu.init (imag::config::BNO08x::i2cAddr))
    {
        DBGLN("Sensor init failed");
        imag::Debug::halt();
    }

    imu.printSensorsPerformingDynamicCalibration();

    // adapt to mounting orientation of sensor
    imu.setReorientation (sensorOrientations[orientationMode]); // initial orientation

    // set data types we are interested in
    if (! imu.setDataTypesToQuery ({ primaryDataType }))
    {
        DBGLN("failed to set custom data types to query");
    }

    // init network transport
    if (! net.init (imag::config::WiFi::ssid, imag::config::WiFi::key, imag::config::WiFi::channel))
    {
        DBGLN("Starting wifi failed");
        imag::Debug::halt();
    }

    net.setTarget (imag::config::Net::remoteIP, imag::config::Net::remotePort);

    // init buttons
    displayButton.begin();
    displayButton.onPressed (toggleDisplay);

    tareButton.begin();
    tareButton.onPressed (setTare);
    tareButton.onPressedFor (2000, resetTare);

    calibButton.begin();
    calibButton.onPressed (switchModeOrEndCalibration);
    calibButton.onPressedFor (2000, beginOrCancelCalibration);
}


void loop()
{
    auto now = millis();
    static auto displayRateTime = now;
    static auto connMsgTime = now;

    // flag for any received data
    auto dataReceived = false;

    // check for int line
    // if (digitalRead (imag::config::BNO08x::intPin) == LOW && intHigh == true)
    // {
    // intHigh = false;
    // DBGLN ("INT falling edge");
    // }
    // else if (digitalRead (imag::config::BNO08x::intPin) == HIGH && intHigh == false)
    // {
    // intHigh = true;
    // DBGLN ("INT rising edge");
    // }

    // update/check current network status
    net.updateConnectionState();

    if (! net.isReadyToSend() && now > connMsgTime)
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
    while (imu.read())
    {
        dataReceived = true;

        // accumulate reliability and accuracy for smoothing
        if (! imu.isCalibrating() && imu.getLastDataType() == primaryDataType)
	{
            reliability.add (imu.getCurrentReliability());
            accuracy.add (imu.getCurrentAccuracy());
	}
        else if (imu.isCalibrating() && imu.getLastDataType() == imag::imu::DataType::mag)
	{
            reliability.add (imu.getCurrentReliability());
	}

        // send data
        if (imag::imu::isAnyRotationDataType (imu.getLastDataType()))
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

            // if (! success)
            //     DBGLN("Error sending MIDI data");

            // skip network sending part if disconnected or calibrating
            if (imu.isCalibrating() || ! net.isReadyToSend())
                continue;

            // send osc
            if (! net.sendQuaternion (imag::osc::Address::rotation, rot))
                DBGLN("Sending osc message failed");
	}
        else
	{ 
            // DBGLN("Cannot send non-rotation data type in current version");
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
        DBG("Battery: "); DBGNLN(battery);

        // calculate effective reliability/accuracy to display
        auto rel = reliability.get();
        auto acc = constrain (accuracy.get() * 180.0 / PI, 0.0, 23.0);

        DBG("smoothed reliability: "); DBGNLN(rel);
        DBG("smoothed accuracy: "); DBGNLN(acc);

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
            display.println (net.isReadyToSend() ? "Sending" : "Disconnected");

            // reported accuracy
            display.fillRect (88, 8, 16, int (acc) + 1, SSD1306_WHITE);
	}

        // reliability
        display.fillRect (112, 32 - int (rel * 23) - 1, 16, int (rel * 23) + 1, SSD1306_WHITE);
        display.display();

        // next display refresh
        displayRateTime += 200;
    }

    // wait for sensor interrupt
    while (digitalRead (imag::config::BNO08x::intPin) == HIGH)
    {
        DBGLN("Waiting for sensor interrupt");
        delay (1);
    }

    DBGLN("Interrupt received, starting next cycle");

    // limit querying rate
    // auto elapsed = millis() - now;

    // if (dataReceived)
    //     DBG("data received. ");
    // DBG("loop took [ms]: "); DBGNLN(elapsed);

    // if (dataReceived && elapsed < 10)
    // {
    //     // sleep until next tick of our sensor rate (100 Hz for now)
    //     // waking up seems to take way too long - not usable atm.
    //     //LowPower.idle (10 - elapsed);

    //     delay (10 - elapsed);
    // }
}
