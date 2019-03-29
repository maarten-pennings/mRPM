# mRPM
Maarten's RPM (Revolutions Per Pinute) meter.

## Introduction
This project describes how to make a RPM (revolutions per minute) meter.
It uses an ESP8266, a 4 element 8x8 LED matrix and a line tracking sensor.
Jan Ridders of [Jan Ridders Modelbouw](http://www.ridders.nu/Webpaginas/pagina_toerenteller/tellerframeset.htm) requested this project.
He needs to measure the speed of his Stirling, steam or combustion engines.
The ESP8266 was chosen because it is easy, cheap and known to Maarten; 
this project does not (yet?) use the WiFi capabilities of the ESP8266.

## Demo
See [YouTube](https://youtu.be/PuOR1rizvE4) for a demo of the project.

## Components
The following components are used in the project, total cost below $10.
- [Line tracking sensor](https://www.aliexpress.com/item/Line-tracking-Sensor-For-robotic-and-car-DIY-Arduino-projects-Digital-Out/32654587628.html)
  An infra-red emitter and detector - sub $1
  (half priced [alternative](https://www.aliexpress.com/item/1PCS-TCRT5000-Infrared-Reflective-IR-Photoelectric-Switch-Barrier-Line-Track-Sensor-Module-blue/32818041246.html)).
- [4 element 8x8 LED matrix](https://www.aliexpress.com/item/MAX7219-Dot-Matrix-Module-Microcontroller-4-In-One-Display-with-5P-Line/32880754577.html)
  A 4-in-one MAX7219 based 8x8 Dot Matrix Module - sub $4
  ([alternative](https://www.aliexpress.com/item/MAX7219-Dot-Matrix-Module-For-Arduino-Microcontroller-4-In-One-Display-with-5P-Line/32648450356.html)).
- [NodeMCU/ESP8266](https://www.aliexpress.com/item/NodeMCU-WIFI-module-integration-of-ESP8266-extra-memory-32M-flash-USB-serial-CH340G/32739832131.html)
  NodeMCU V3: ESP8266, 32M flash, CH340G (USB-serial) - sub $4
  (half priced [alternative](https://www.aliexpress.com/item/1pcs-NodeMCU-V3-Lua-WIFI-module-integration-of-ESP8266-extra-memory-32M-Flash-USB-serial-CH340G/32813549591.html)).

## Wiring
For wiring information (power, schematics, photos) refer to the [wiring](wiring) directory.

## Software
The software (firmware inside the ESP8266) is available as binary, 
and can be [flashed](flash.md) relatively easy.
Another option is to install the development environment (compiler) 
and work from the source; refer to the [arduino](arduino) directory.

(end of doc)
