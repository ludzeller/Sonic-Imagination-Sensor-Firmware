# Sonic Imagination - Sensor Firmware

Firmware for Imagination wireless head tracking sensor.

[[_TOC_]]

## Button actions

Refers to the buttons on the Featherwing OLED peripheral board.

- **A** Toggle display enablement. In any case, display will be switched off after 30 seconds (powersave).
- **B** Custom north
  + _Short press:_ Set custom north (current front direction will become the new north reference).
  + _Long press:_ Reset to magnetic north.
- **C** Orientation conventions and calibration
  + _Short press:_ Cycle through orientation conventions, e.g. _std_ (Imagination) or _iem_ (MrHat compatibility).
  + _Long press:_ Enter calibration mode.
  + When in calibration mode:
    * _Short press:_ Save calibration and leave calibration mode.
    * _Long press:_ Discard calibration and leave calibration mode (previous calibration will remain active).

## Calibration procedure

Calibration procedure according to the [BNO08x sensor documention](https://www.ceva-dsp.com/resource/bno080-bno085-sesnor-calibration-procedure/):

- Enter calibration mode (long-press button C)
- Place the sensor stable and immobile for a few seconds, e.g., by putting on a table (gyroscope calibration)
- Place the sensor in 4..6 different, distinct directions and leave immobile for some seconds, e.g., the six plane areas of a cuboid (accelerometer calibration)
- Rotate the sensor around different axes evenly by 180 degrees and back over approx. 2 sec. per way (magnetometer calibration)
- The reliability measure displayed in the rightmost bar should become as high as possible
- Save calibration (short-press button C)

The calibration settings will be persistent across power cycles.

## Hardware

The current prototype is made of
- [Adafruit Feather M0 WiFi](https://www.adafruit.com/product/3010) (SAMD21 + ATWINC1500)
- [Adafruit BNO085 IMU Fusion Breakout](https://www.adafruit.com/product/4754) (I2C connected)
- [Adafruit Featherwing OLED 128x32](https://www.adafruit.com/product/3045) (Display and button I/O)

## Library dependencies

The firmware can be compiled using the Arduino IDE.

Please refer to the respective Adafruit documentation for installing the board files and support libraries:
- [Feather M0 WiFi](https://learn.adafruit.com/adafruit-feather-m0-wifi-atwinc1500)
- [BNO085 IMU](https://learn.adafruit.com/adafruit-9-dof-orientation-imu-fusion-breakout-bno085)
- [Featherwing OLED](https://learn.adafruit.com/adafruit-oled-featherwing)

Additionally, the following libraries are used (installable via Arduino library manager):
- [LiteOSCParser](https://github.com/ssilverman/LiteOSCParser)
- [MIDIUSB](https://www.arduino.cc/en/Reference/MIDIUSB)
- [EasyButton](https://easybtn.earias.me/)
- ~~[Arduino Low Power](https://www.arduino.cc/en/Reference/ArduinoLowPower)~~ (currently not used)

Finally, this library needs to be installed manually (copied/checked out to Arduino libraries directory):
- [Arduino-Helpers](https://github.com/tttapa/Arduino-Helpers) (Quaternion implementation)

## Version history

- _0.1.0_ initial release
