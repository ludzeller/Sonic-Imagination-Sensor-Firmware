/* sensor_firmware_feather_m0

   imagination sensor firmware
   main

   2021-2024 rumori
*/

#include "imag_config.h"
#include "imag_debug.h"
#include "imag_osc_address.h"
#include "imag_smoother.h"
#include "imag_battery.h"

#include "imag_imu_bno08x.h"
#include "imag_osc_winc150x.h"
#include "imag_display_sh1107.h"

#include <MIDIUSB.h>
#include <EasyButton.h>

// string constants
static const String versionString { String (imag::config::versionMajor) + "." + imag::config::versionMinor + "." + imag::config::versionSub };
static const String ssid { String (imag::config::WiFi::ssid) + "_" + imag::config::sensorIndex };

// members
imag::imu::BNO08x imu { imag::config::BNO08x::resetPin, imag::config::BNO08x::intPin };

const auto localAddr = imag::config::Net::localIP;
imag::osc::WINC150x net { localAddr, imag::config::Net::localPort };

imag::display::SH1107 oled { &Wire };

imag::Battery battery { imag::config::Battery::pin, true };

// buttons
EasyButton buttonA (imag::config::Button::pinA, imag::config::Button::debounce, true, true);
EasyButton buttonB (imag::config::Button::pinB, imag::config::Button::debounce, true, true);
EasyButton buttonC (imag::config::Button::pinC, imag::config::Button::debounce, true, true);

// convenience array
static const std::array<EasyButton*, 3> buttons { { &buttonA, &buttonB, &buttonC } };

// sensor data types
static constexpr auto primaryDataType = imag::imu::DataType::rotationArvr;

// sensor mounting orientation/output conventions
static const std::array<const Quaternion, 2> sensorOrientations {
    {
        { 1.0f, 0.0f, 0.0f, 0.0f },             // normal mounting, no reorientation
        { 0.7071068f, 0.0f, 0.0f, -0.7071068f } // normal mounting, no reorientation w/ iem mrhat compat rotation
        // { 0.0f, -1.0f, 0.0f, 0.0f },         // mounted upside down front
        // { 0.0f, 0.0f, -1.0f, 0.0f },         // mounted upside down back : imagination prototype mounting
        // { 0.0f, -0.7071f, 0.7071f, 0.0f }    // mounted upside down back w/ iem mrhat compat rotation
    }
};

static_assert (sensorOrientations.size() == imag::display::Message::Main::orientationConfig.size());

// current orientation mode
uint8_t orientationMode = 0;

// custom north offset
bool customNorth = false;
Quaternion customNorthOffset = Quaternion::identity();

// reliability && accuracy smoothers
static constexpr auto smoothLen = 100;
static imag::Smoother<float, smoothLen> reliability, accuracy;

void attachButtonsNorm();
void attachButtonsCalibration();

void beginCalibration()
{
    if (oled.setEnabled (true))
        return;

    if (! imu.isCalibrating())
        imu.beginCalibration();

    imu.printSensorsPerformingDynamicCalibration();
    attachButtonsCalibration();
}


void endCalibration()
{
    if (imu.isCalibrating())
        imu.endCalibration();

    imu.printSensorsPerformingDynamicCalibration();
    attachButtonsNorm();
}


void saveCalibration()
{
    endCalibration();
    imu.saveCalibration();
}


void clearCalibration()
{
    endCalibration();
    imu.clearCalibration();
}


void cycleOrientationMode()
{
    if (oled.setEnabled (true))
        return;

    // rotate mode
    orientationMode = ++orientationMode % sensorOrientations.size();
    imu.setReorientation (sensorOrientations[orientationMode]);
}


void setReorientation()
{
    if (oled.setEnabled (true))
        return;

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


void resetReorientation()
{
    if (oled.setEnabled (true))
        return;
  
    customNorthOffset = Quaternion::identity();
    customNorth = false;
}


void toggleDisplay()
{
    oled.setEnabled (! oled.isEnabled());
}


void nop() {}


// attach buttons, normal mode
void attachButtonsNorm()
{
    buttonA.onPressed (toggleDisplay);

    buttonB.onPressed (setReorientation);
    buttonB.onPressedFor (2000, resetReorientation);

    buttonC.onPressed (cycleOrientationMode);
    buttonC.onPressedFor (2000, beginCalibration);
}


// attach buttons, calibration mode
void attachButtonsCalibration()
{
    buttonA.onPressed (clearCalibration);

    buttonB.onPressed (endCalibration);
    buttonB.onPressedFor (2000, nop);

    buttonC.onPressed (saveCalibration);
    buttonC.onPressedFor (2000, nop);
}


// update battery status to display content
void updateBattery()
{
    battery.update();
    oled.getContent().batteryVoltage = battery.getVoltage();
    oled.getContent().batteryPercentage = battery.getPercentage();
}


void setup()
{
    Wire.setClock(200000L);   // I2C speed, 400 kHz is a little fast for Arduino's pullups

    // led
    pinMode (LED_BUILTIN, OUTPUT);
    digitalWrite (LED_BUILTIN, LOW);

    delay (250); // give the baby some time to settle

    // oled display
    oled.init (imag::config::Display::i2cAddr);
    oled.getContent().version = versionString.c_str();
    oled.getContent().ssid = ssid.c_str();
    updateBattery(); // read initially for low bat splash warning
    oled.setPage (imag::display::Page::splash);
    oled.update();

    delay (100);

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
    if (! net.init (ssid.c_str(), imag::config::WiFi::key, imag::config::WiFi::channel))
    {
        DBGLN("Starting wifi failed");
        imag::Debug::halt();
    }

    net.setTarget (imag::config::Net::remoteIP, imag::config::Net::remotePort);

    // init buttons
    for (auto* button : buttons)
        button->begin();

    attachButtonsNorm();
}


void loop()
{
    auto now = millis();
    static auto connMsgTime = now;

    // flag for any received data
    auto dataReceived = false;

    // update/check current network status
    net.updateConnectionState();

    if (! net.isReadyToSend() && now > connMsgTime)
    {
        // output message every 2s
        DBGLN("Waiting for wifi connection...");
        connMsgTime += 2000;
    }

    // eval buttons
    for (auto* button : buttons)
        button->read();

    // display refresh/timeout
    oled.setPage (imu.isCalibrating() ? imag::display::Page::calibration : imag::display::Page::main);
    oled.update();

    // battery read cycle if due
    updateBattery();

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

            // update display data
            oled.getContent().orientationConfig = orientationMode;
            oled.getContent().customNorth = customNorth;
            oled.getContent().rotation = rot;
            oled.getContent().senderOsc = net.isReadyToSend();
            oled.getContent().senderMidi = success;
            
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

    { // update remaining display data
        oled.getContent().reliability = reliability.get();
        oled.getContent().accuracy = constrain (accuracy.get(), 0.0f, 0.5f * PI) / (0.5f * PI); // constrain to 0..90 deg

        DBG("smoothed reliability: "); DBGNLN(oled.getContent().reliability);
        DBG("smoothed accuracy: "); DBGNLN(oled.getContent().accuracy * 90.0f);

        if (imu.isCalibrating())
            oled.resetAutoOff(); // do not auto-off when calibrating
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

}
