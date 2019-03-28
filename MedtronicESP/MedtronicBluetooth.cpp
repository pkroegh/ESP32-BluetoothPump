#include <Arduino.h>
#include "MedtronicBluetooth.h"
#include "ASCII.h"
//************************************************************************************
// Public functions
//************************************************************************************
// Start bluetooth communication
MedtronicBluetooth::MedtronicBluetooth(String deviceName, bool setDebug = false) {
    SerialBT.begin(deviceName); // Starts bluetooth serial
    delay(200); // wait for voltage stabilize
    if(setDebug) {
        this->debugBluetooth = true;
        debug(1);
    } 
}
// If any, get bluetooth message
String MedtronicBluetooth::getMessage() {
    return readBluetooth();
}
// Send bluetooth message
void MedtronicBluetooth::sendMessage(String message) {
    sendBluetooth(message);
}
//************************************************************************************
// Private functions
//************************************************************************************
// Read from bleutooth serial and add to buffer
String MedtronicBluetooth::readBluetooth(String dataString) {
    boolean dataAvaliable = false;
    uint8_t btData;
    while (SerialBT.available()) { // Run when data in buffer
        dataAvaliable = true;
        btData = SerialBT.read(); // Add data to variable
        if (btData == 13) { // New line marks end of string
            return dataString; // Return the string
        }
        dataString.concat(ASCIIintToChar(btData)); // Add data to string
    }
    if (dataAvaliable && btData != 13) { // String not fully recieved
        delay(10); // Wait a short time
        readBluetooth(dataString); // Return to bluetooth reader
    }
    return null;
}
// Transmit bluetooth message to host
void MedtronicBluetooth::sendBluetooth(String message) {
    for (uint8_t i = 0; i < message.length(); i++){
        if (this->debugBluetooth) {
            char buff = message[i];
            debug(2,buff)
        }
        SerialBT.write(message[i]);
    }
    SerialBT.write(13);
}
// Print debug info
void MedtronicBluetooth::debug(uint8_t debuglvl, char character = null) {
    switch (debuglvl) {
        case 1:
            Serial.print("BluetoothSerial started with device name: ");
            Serial.println(deviceName);
        break;
        case 2:
            if (character != null) {
                Serial.print(character);
            }
        break;
    }
}