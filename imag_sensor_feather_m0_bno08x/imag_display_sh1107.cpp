/* imag_display_sh110x.cpp
 * 
 * imagination sensor firmware
 * sh110x 128x64 display interface
 * 
 * 2021-2024 rumori
 */

#include "imag_display_sh1107.h"
#include "imag_debug.h"

// redefine DBG output macros for this module only
#if ! IMAG_DISPLAY_DEBUG
#undef DBG
#define DBG       ;
#undef DBGLN
#define DBGLN     ;
#undef DBGN
#define DBGN      ;
#undef DBGNLN
#define DBGNLN    ;
#undef DBGHEX
#define DBGHEX    ;
#endif // #if ! IMAG_DISPLAY_DEBUG

namespace imag::display
{

SH1107::SH1107 (TwoWire* twi)
    : display (displayHeight, displayWidth, twi),
      page (Page::splash),
      displayOn (true),
      refreshTime (0)
{
    resetAutoOff();
}


bool SH1107::init (uint8_t i2cAddr, bool reset)
{
    if (! display.begin (i2cAddr, reset))
    {
        DBGLN("SH1107: display begin() failed");
        return false;
    }
    
    display.cp437 (true); // correct symbol character code page
    display.setRotation (1);
    display.setTextColor (SH110X_WHITE);

    return true;
}


void SH1107::update()
{
    if (! isEnabled())
        return;

    const auto now = millis();
    
    // refresh cycle?
    if (now < refreshTime)
        return;
    
    // shall we auto off?
    if (now > autoOffTime)
    {
        setEnabled (false);
        return;
    }

    // schedule next refresh
    refreshTime = now + displayRefresh;

    if (page == Page::splash)
        showSplash();
    else if (page == Page::main)
        showMain();
    else if (page == Page::calibration)
        showCalibration();
}


bool SH1107::setEnabled (bool shouldBeOn)
{
    auto res = displayOn != shouldBeOn;
    
    displayOn = shouldBeOn;

    if (shouldBeOn)
    {
        display.oled_command (SH110X_DISPLAYON);
        resetAutoOff();
    }
    else // switch off
    {
        display.oled_command (SH110X_DISPLAYOFF);
    }

    return res;
}

void SH1107::showSplash()
{
    display.clearDisplay();
    display.setCursor (0, 0);

    display.setTextSize (2);
    display.println (Message::Splash::title);

    display.setTextSize (1);
    display.print (Message::Splash::versionLabel);
    display.println (content.version);

    display.println();

#if IMAG_DEBUG
    display.println (Message::Splash::debugSerialWait);
#else
    display.println (Message::Splash::initialising);
#endif

    display.display();
}


void SH1107::showMain()
{
    static constexpr auto charWidth = 6;
    static constexpr auto lineSkip = 11;
    
    display.clearDisplay();
    display.setTextSize (1);

    // wlan ssid
    display.setCursor (0, 0);
    display.print (content.ssid);

    { // sender
        display.setCursor (0 * charWidth, lineSkip);
        display.print (Message::Main::senderOsc);
        display.setCursor (strlen (Message::Main::senderOsc) * charWidth + charWidth / 2, lineSkip);        
        display.print (content.senderOsc ? Message::asterisk : Message::underscore);

        display.setCursor (6 * charWidth + charWidth / 2, lineSkip);
        display.print (Message::Main::senderMidi);
        display.setCursor ((7 + strlen (Message::Main::senderMidi)) * charWidth + charWidth / 2, lineSkip);        
        display.print (content.senderMidi ? Message::asterisk : Message::underscore);
    } // sender
    
    { // button stuff
        // instantiate constexpr message members (compiler flaw)
        static const auto buttonLabel { Message::Main::buttons };
        static const auto orientationConfig { Message::Main::orientationConfig };

        static constexpr auto labelWidth = 6;
        static constexpr auto textX = displayWidth - ((labelWidth + 1) * charWidth + 2);

        // button legends
        for (size_t i = 0; i < buttonLabel.size(); ++i)
        {
            display.setCursor (textX, i * 28);
            display.print (buttonLabel[i]);
            display.setCursor (displayWidth - 6, i * 28);
            display.write (0x1a);
        }

        // orientation config
        display.setCursor (textX, 0 * 28 + lineSkip);
        display.print (orientationConfig[content.orientationConfig]);

        // north setting
        display.setCursor (textX, 1 * 28 + lineSkip);
        display.print (content.customNorth ? Message::Main::customNorth : Message::Main::magneticNorth);
    } // button stuff

    { // battery
        display.setCursor (0, displayHeight - 8);
        display.print (Message::Battery::label);
        display.print (content.battery);
        display.print ("%");
    } // battery

    { // reliabilty/accuracy
        static constexpr auto maxLength = 42;
        static constexpr auto vOffset = 2;
        const auto relLength = round (content.reliability * maxLength);
        const auto accLength = round (content.accuracy * maxLength);
        
        display.setCursor (0 + charWidth / 2, 1 * 28);
        display.print (Message::Main::reliability);
        display.drawFastVLine (0, 1 * 28 - vOffset, lineSkip, SH110X_WHITE);
        display.drawFastHLine (0, 1 * 28 - vOffset, relLength, SH110X_WHITE);
        
        display.setCursor (4 * charWidth - charWidth / 2, 1 * 28 + lineSkip);
        display.print (Message::Main::accuracy);
        display.drawFastVLine (maxLength, 1 * 28 + lineSkip - vOffset + 1, lineSkip, SH110X_WHITE);
        display.drawFastHLine (maxLength - accLength, 1 * 28 + 2 * lineSkip - vOffset, accLength, SH110X_WHITE);
    } // reliabilty/accuracy

    { // north direction indicator
        static constexpr uint16_t centreX = 63;
        static constexpr uint16_t centreY = 44;
        static constexpr auto radius = 13.0f;
        static constexpr auto opening = 17.0f * PI / 180.0f;

        const auto north = PI - EulerAngles (content.rotation).yaw;
        
        const auto backCorner1 = north + PI + opening;
        const auto backCorner2 = north + PI - opening;
        
        display.fillTriangle (centreX + round (sin (north) * radius),
                              centreY + round (cos (north) * radius),
                              centreX + round (sin (backCorner1) * radius),
                              centreY + round (cos (backCorner1) * radius),
                              centreX + round (sin (backCorner2) * radius),
                              centreY + round (cos (backCorner2) * radius),
                              SH110X_WHITE);
    } // north direction indicator

    display.display();
}


void SH1107::showCalibration()
{
    static constexpr auto charWidth = 6;
    static constexpr auto lineSkip = 11;
    
    display.clearDisplay();
    display.setTextSize (1);

    // wlan ssid
    display.setCursor (0, 0);
    display.print (content.ssid);

    { // calibration label
        display.setCursor (0 * charWidth, lineSkip);
        display.print (Message::Calibration::label);
    }
    
    { // button stuff
        // instantiate constexpr message members (compiler flaw)
        static const auto buttonLabel { Message::Calibration::buttons };

        static constexpr auto labelWidth = 6;
        static constexpr auto textX = displayWidth - ((labelWidth + 1) * charWidth + 2);

        // button legends
        for (size_t i = 0; i < buttonLabel.size(); ++i)
        {
            display.setCursor (textX, i * 28);
            display.print (buttonLabel[i]);
            display.setCursor (displayWidth - 6, i * 28);
            display.write (0x1a);
        }
    } // button stuff

    { // battery
        display.setCursor (0, displayHeight - 8);
        display.print (Message::Battery::label);
        display.print (content.battery);
        display.print ("%");
    } // battery

    { // calibration reliabilty
        static constexpr auto maxLength = 80;
        const auto relLength = round (content.reliability * maxLength);
        
        display.setCursor (0, 1 * 28);
        display.print (Message::Calibration::reliability);

        display.drawRect (0, 1 * 28 + lineSkip, maxLength, lineSkip, SH110X_WHITE);
        display.fillRect (0, 1 * 28 + lineSkip, relLength, lineSkip, SH110X_WHITE);
    } // calibration reliabilty

    display.display();
}

} // namespace imag::display
