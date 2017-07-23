# mRPM
Maarten's RPM meter.

## Introduction
This project describes how to make a RPM (rotations per minute) meter.
It uses an ESP8266, a 4 element 8x8 LED matrix and a line tracking sensor.
Jan Ridders of [Jan Ridders Modelbouw](http://www.ridders.nu/) requested this project.
He needs to measure the speed of his stirling, steam or combustion engines.
The ESP8266 was chosen because it is easy and cheap; this project does not (yet?) use the WiFi capabilities of the ESP8266.

## Demo
See [YouTube](https://youtu.be/PuOR1rizvE4) for a demo of the project.

## Components
The following components are used in the project, total cost below $10.
- [Line tracking sensor](https://www.aliexpress.com/item/Line-tracking-Sensor-For-robotic-and-car-DIY-Arduino-projects-Digital-Out/32654587628.html)
  An infrared emitor and detector and a potentiometer to adjust the sensitivity - sub $1.
- [4 element 8x8 LED matrix](https://www.aliexpress.com/item/1Pcs-MAX7219-Dot-Matrix-Module-For-arduino-Microcontroller-4-In-One-Display-with-5P-Line/32624431446.html)
  A 4-in-one MAX7219 based 8x8 Dot Matrix Module - sub $4.
- [NodeMCU/ESP8266](https://www.aliexpress.com/item/NodeMCU-WIFI-module-integration-of-ESP8266-extra-memory-32M-flash-USB-serial-CH340G/32739832131.html)
  NodeMCU V3: ESP8266, 32M flash, CH340G (USB-serial) - sub $4


