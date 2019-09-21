# ESP32-BluetoothPump
Source code for an ESP32 intended to be connected to a Medtronic MMT-554 pump, allowing for regulation of temp basal rate. The ESP is intended to be pared with a modificed version of the AndroidAPS app. AndroidAPS utilizes CGM data to calculate the optimal temp basal rate to be set to maintain a stable blood sugar level. The modified version of AndroidAPS allows for transmission of the temp basal rate to the ESP. The ESP can then set the temp basal rate in the Medtronic MMT-554 pump, using the physical pump interface. 

## Setup

### Downloading and installing the modificed AndroidAPS app
Download and install [Android Studio](https://developer.android.com/studio)

Clone modified version of AndroidAPS, by selecting ```Check out projekt from Version Control```. In the promt input the GitHub clone path for the modified AndroidAPS, https://github.com/pkroegh/AndroidAPS-Bluetooth.git.

Connect your Android device to your computer.
Once the projekt is open, select ```Run > Run``` and select "app" in the prompt. Select your Android device when prompted. This will compile the app and install it on your Android device.

### Connecting the ESP32 to the Medtronic MMT-554 pump
**Setting up Arduino and ESP**

If your have not already install [Arduino IDE](https://www.arduino.cc/en/main/software).

Then add the ESP32 to your Arduino IDE installation, following this [guide](https://github.com/espressif/arduino-esp32/blob/master/docs/arduino-ide/boards_manager.md) from Espressif.

**Compiling the software and uploading it to the ESP**

First, connect the ESP32 to your computer.
Clone this repository and open the ```.ino``` file. Select the ESP32 board in the ```Tools > Board```. Select the correct USB port ```Tools > Port```. Finally, press the upload button or press ```Ctrl+U```.

**Setting up the physical interface**

To connect the ESP32 to the Medtronic pump, we prealed off the plastic cover that sits above the buttons. This reveals a PCB with six traces. One trace touching the outer part of all button pads and five seperate trace that touch the each of the five buttons. We soldered one wire to the common trace and five seperate wires to the remaining traces. For each of the button wires we connected the common wire to pin six and the button wire to pin four of a AQV21 solid state relay PhotoMOS. This chips is a type of optocoupler using CMOS technology. (As long as the chip features a galvanig isolation between the relay and relay activation diode, it should work for the application). The galvanig isolation makes sure that the ESP32 and pump power supplies remain seperate. Pin two of the AQV21 we connected to a GND of the ESP32 and the AQV21 pin one we connected to the ESP32 digital output in the following order:
 Markup : * ESP pin -> Pump button
          * Pin 15 -> Bolus button
          * Pin 14 -> ACT button
          * Pin 32 -> Escape button
          * Pin 27 -> Up button
          * Pin 33 -> Down button
(This method of interfacing with the pump is not the best of ways, but functional nevertheless. We would love to know if you come up with a smarter way of interfacing with the pump).

## Modified AndroidAPS
The [modified version of AndroidAPS](https://github.com/pkroegh/AndroidAPS-Bluetooth) , ment to be pared with the ESP32.

## Original AndroidAPS 
The  [original AndroidAPS](https://github.com/MilosKozak/AndroidAPS) by MilosKozak.

The [documentation](https://androidaps.readthedocs.io/en/latest/EN/) for AndroidAPS. 
