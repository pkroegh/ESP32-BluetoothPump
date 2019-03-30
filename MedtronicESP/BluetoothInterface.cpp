#include <Arduino.h>
#include "BluetoothInterface.h"
#include "ASCII.h"
//************************************************************************************
// Public functions
//************************************************************************************
// Initialize library
BluetoothInterface::BluetoothInterface() {
    this->_deviceName = "ESP32"; //Set default name
}
// Start bluetooth communication
void BluetoothInterface::begin(String name) {
    SerialBT.begin(name); // Starts bluetooth serial
    delay(200); // wait for voltage stabilize
}
// Start bluetooth communication with debug
void BluetoothInterface::begin(String name, bool debug) {
    SerialBT.begin(name); // Starts bluetooth serial
    delay(200); // wait for voltage stabilize
    this->_debug = debug;
    if(this->_debug) {
        Serial.print("BluetoothSerial started with device name: ");
        Serial.println(name);
    }
}
// If any, get bluetooth message
String BluetoothInterface::getMessage() {
    return readBluetooth();
}
// Send bluetooth message
void BluetoothInterface::sendMessage(String message) {
    sendBluetooth(message);
}
//************************************************************************************
// Private functions
//************************************************************************************
// Read from bleutooth serial and add to buffer
String BluetoothInterface::readBluetooth(String dataString) {
    boolean dataAvaliable = false;
    uint8_t btData;
    while (SerialBT.available()) { // Run when data in buffer
        dataAvaliable = true;
        btData = SerialBT.read(); // Add data to variable
        if (btData == 13) { // New line marks end of string
            if (this->_debug) { Serial.print("Got string: ");
                Serial.println(dataString); }
            return dataString; // Return the string
        }
        dataString.concat(ASCIIintToChar(btData)); // Add data to string
    }
    if (dataAvaliable && btData != 13) { // String not fully recieved
        delay(10); // Wait a short time
        readBluetooth(dataString); // Return to bluetooth reader
    }
    return "";
}
// Transmit bluetooth message to host
void BluetoothInterface::sendBluetooth(String message) {
    if (this->_debug) { Serial.print("Transmitting message: "); }
    for (uint8_t i = 0; i < message.length(); i++){
        if (this->_debug) { char buff = message[i]; Serial.print(buff); }
        SerialBT.write(message[i]);
    }
    if (this->_debug) { Serial.println(""); }      
    SerialBT.write(13);
}