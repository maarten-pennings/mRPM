# mRPM
Maarten's RPM meter - arduino firmware


## Introduction
This project uses the Arduino IDE to compile and upload an image for the ESP8266.
The mRPM firmware consists of the following modules
 - mRPM, the main program
 - MAX7219_Dot_Matrix, a driver for the 4 x 8x8 dot matrix - by [Nick Gammon](https://github.com/nickgammon/MAX7219_Dot_Matrix)
 - bitBangedSPI, the SPI driver use dby the dot matrix driver - by [Nick Gammon](https://github.com/nickgammon/bitBangedSPI)
 - a font for the matrix - by [Nick Gammon](https://github.com/nickgammon/MAX7219_Dot_Matrix)

The latter three modules are written by [Nick Gammon](https://github.com/nickgammon).
Experienced Arduino programmers will store those modules in their library.
For easy integration, they are copied into this project.

Note that the font matrix has been patched. It contains 11 special characters: the digits '0' to '9' and the dash '-', all with a trailing decimal dot.

 
## Steps


### Download and install Arduino for ESP8266
There is an official [Arduino web page](http://www.arduinesp.com/getting-started) explaining how to use the ESP8266 with the Arduino IDE.
However, it uses a bare ESP8266 module; this project uses an ESP8266 module on a so-called nodemcu board (basically adding a USB to UART bridge).
This nodemcu specific [instructables](http://www.instructables.com/id/Quick-Start-to-Nodemcu-ESP8266-on-Arduino-IDE/) guide might be more appropriate.

In either case
 - Install the Arduino IDE.
 - Add the ESP8266 compiler/libraries/examples.
 - Get the blink example to work (this requires the ESP8266 board to be connected to your PC with a micro USB cable).
 - Change the blink example a bit (e.g. make it blink faster) and save the (edited) blink project.

Typically the project is saved at `C:\Users\Maarten\Documents\Arduino\Blink`.
Write down the root directory for the Arduino projects
```
C:\Users\Maarten\Documents\Arduino
```


### Download and install this project
Follow these steps
 - Download the zip of this project (press the green button on the [home page](https://github.com/maarten-pennings/mRPM)).
 - Open the zip, locate the directory `arduino` and within that the subdirectory `mRPM`.
   ```
   C:\Users\Maarten\Downloads\mRPM-master.zip\mRPM-master\arduino\mRPM
   ```
- Copy the `mRPM` directory to the Arduino project root.
   ```
   C:\Users\Maarten\Documents\Arduino\mRPM
   ```


### Open, compile, flash
The ESP8266 board is assumed to be wired to the matrix and sensor - see [wiring](../wiring) for instructions.

Next, follow these steps
 - Double click the project `C:\Users\Maarten\Documents\Arduino\mRPM\mRPM.ino` to open it in the Arduino IDE.
 - Compile it by pressing the Verify (green V in upper left corner) button.
 - Flash it by it by pressing the Upload (green right arrow) button.
 - Quickly waive a white sheet of paper in front of the sensor. Its blue led should flash, and the matrix should show RPM numbers.
 - On the ESP8266 board, press the small button labeled Flash (not RST) to change units.
 
 Congratulations, you're done!
 
