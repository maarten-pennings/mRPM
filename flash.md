# Flashing
This document describes how to flash a binary released firmware into the ESP8266.


## Introduction
To flash the a firmware image into an ESP8266, we need
 - A flash tool
 - A firmware image
 - A connected ESP8266
 - To execute flash command

This description assumes the PC runs Windows 10.
If you have an older version of Windows you need to install a driver for 
the ESP8266/NodeMCU board. That is not covered in this document.


## The flash tool
To flash the ESP8266 a tool is needed.
We will be using the `esptool.exe`.

If the Arduino IDE with ESP8266 support is installed on your Windows PC:
 - Locate the tool. On my system it is located at
   `C:\Users\maarten\AppData\Local\Arduino15\packages\esp8266\tools\esptool\0.4.9\esptool.exe`
 - Create a directory named `flash` on the Desktop.
 - Copy the `esptool.exe` to that directory.

If there is no Arduino on your PC, follow these steps
 - Goto [esptool-ck](https://github.com/igrr/esptool-ck/releases).
 - Scroll to find latest stable release.
   At the moment of writing this document the latest release is 0.4.9.
 - Click the Windows `zip` file (e.g. `esptool-0.4.9-win32.zip`) to download it.
 - Save on e.g. Desktop.
 - Unzip "Here".
 - Rename directory `esptool-0.4.9-win32` to `flash`.
 
In either case there is now a directory named `flash` on the Desktop.
Inside that directory there is `esptool.exe`.


## The firmware image
To flash the ESP8266 a firmware image is needed.
We will be using a binary release from this project.

Steps
 - Go to the [release](https://github.com/maarten-pennings/mRPM/releases) 
   tab on the projects home page.
 - Scroll to find latest stable release.
   At the moment of writing this document the latest release is v4.
 - Click the `bin` file (e.g. `mRPM.ino.bin`) to download it.
 - Save in the `flash` directory (on the Desktop), created in the previous section.

 
## ESP8266 connection
To flash the ESP8266 it needs to be connected to the PC using a USB cable.

Steps:
 - Start the `Device Manager`.
   On Windows 10, right-click on the start menu 
   (the "Orb" in the lower left corner) and select `Device Manager`.
 - Open the section `Ports (COM & LPT)`.
 - Plug USB cable in ESP8266/NodeMCU board and in the PC.
 - An entry appears in `Ports (COM & LPT)` 
   (if the list is long it might help to unplug and replug to see it disappear and reappear).
 - The entry looks like `USB-SERIAL CH340 (COM3)`.
   Note down the COM port number, here `COM3`.


## Execute flashing
Now that everything is prepared, we are ready to flash.

Steps:
 - Make sure the ESP8266 is still connected with the USB cable to the PC.
 - While holding shift down, right-click on the `flash` directory on the Desktop
   and select `Open command window here`.
 - In the terminal that pops up enter this command
   ```
   esptool  -cd nodemcu  -cb 512000  -cp COM3  -cf mRPM.ino.bin
   ```
   Replace `COM3` with the COM port found in the previous section.
 - You should see
   ```
   C:\Users\maarten\Desktop\flash>esptool  -cd nodemcu  -cb 512000  -cp COM3  -cf mRPM.ino.bin
   Uploading 235264 bytes from mRPM.ino.bin to flash at 0x00000000
   ................................................................................ [ 34% ]
   ................................................................................ [ 69% ]
   ......................................................................           [ 100% ]
   C:\Users\maarten\Desktop\flash>
   ```

You should now have this [result](https://youtu.be/PuOR1rizvE4).
 
Congratulations, you're done!

 
(end of doc)
