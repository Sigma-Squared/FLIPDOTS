<h1 align="center">FLIPDOTS Clock</h1>
<p align="center">
<img src="/docs/anim.gif" alt="Clock gif">
</p>

## Introduction
This is a clock built using an ESP32 and a 7x7 flipdot display by [Alfa-Zeta](https://flipdots.com/en/home/). It synchronizes time over WiFi using NTP.

## Hardware
### Parts List
 - ESP32 (I used a DOIT ESP32 DevKit V1)
 - Alfa-Zeta 7x7 FLIPDOTS display
 - MAX485 based RS485 serial tranciever (for communication with display)
 - DC-DC Buck converter (for stepping down 24v power supply needed for display to 3.3v for logic)
 - 24v 1.5A power supply (although you could get away with much less)
### Schematics
<img src="/docs/FLIPDOTS_bb.png" alt="Android serial settings" width="400"> <img src="/docs/FLIPDOTS_schem.png" alt="Android serial settings" width="400">
### Enclosure
The enclosure was designed in Fusion 360 and Laser cut on 5.33mmm plywood, and stained black.

#### SVG for Laser Cutting
<img src="/docs/enclosure.svg" alt="Android serial settings" width="400">

#### Construction
<img src="/docs/enclosure_construction.png" alt="Android serial settings" width="400">

## Software
This is a [PlatformIO](https://platformio.org/) project and needs PlatformIO to compile. It includes a custom written FLIPDOTS library for graphics/fonts and interfacing with the display. It uses the ESP32's RTOS [freeRTOS](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/freertos.html) for scheduling the time updater and using the dual cores of the microcontroller to multitask (like updating the display while doing WiFi/bluetooth things)
### Configuration
WiFi credentials, timezone, and daylight savings offset is attempted to be set over Bluetooth each time the system boots. If it is successfully configured and connected, it will save that information into the flash storage for use on later boots. If Bluetooth is not connected for 10 seconds on startup, the device will load its last successful configuration from flash.
For example, here it's being set over a Bluetooth serial app on an Android phone:

<img src="/docs/setup.png" alt="Android serial settings" width="600">
