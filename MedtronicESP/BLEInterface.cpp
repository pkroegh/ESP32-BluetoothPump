#include <Arduino.h>
#include "BLEInterface.h"
//************************************************************************************
// Public functions
//************************************************************************************
// Setup pinout
BLEInterface::BLEInterface(String *name, void (*callback)(String)) {
    _name = name;
    _callback = callback;
}
// Start BLE
void BLEInterface::begin() {
    setupBLE();
}
// Start BLE with debug
void BLEInterface::begin(bool debug) {
    _debug = debug;
    setupBLE();
}
// Send bluetooth message
bool BLEInterface::sendMessage(String message) {
    if (_deviceConnected) {
        uint8_t size = message.length()+1;
        char charMessage[size];
        message.toCharArray(charMessage, size);
        charMessage[size] = '\0';
        _pOutputCharacteristic->setValue(charMessage);
        _pOutputCharacteristic->notify(); // Send the value to the app!
        if (_debug) {
            Serial.print("*** Sent string: ");
            Serial.println(message);
        }
        return true;
    }
    return false;
}
void BLEInterface::end() {
    stopBLE();
}
//************************************************************************************
// Private functions
//************************************************************************************
// Setup BLE server and service 
void BLEInterface::setupBLE() {
    // Create the BLE Device
    uint8_t size = _name->length()+1;
    char charName[size];
    _name->toCharArray(charName, size);
    charName[size] = '\0';
    BLEDevice::init(charName); // Give it a name
    // Create the BLE Server
    _pServer = BLEDevice::createServer();
    _pServer->setCallbacks(new MyServerCallbacks(&this->_deviceConnected, &this->_debug));
    // Create the BLE Service
    BLEService *_pService = _pServer->createService(SERVICE_UUID);
    // Create input BLE Characteristic
    _pInputCharacteristic = _pService->
        createCharacteristic(CHARACTERISTIC_UUID_RX, BLECharacteristic::PROPERTY_WRITE);
    // Create output BLE Characteristic
    _pOutputCharacteristic = _pService->
        createCharacteristic(CHARACTERISTIC_UUID_TX, BLECharacteristic::PROPERTY_NOTIFY 
                                                   | BLECharacteristic::PROPERTY_READ);
    // Add descriptors
    _pInputCharacteristic->setCallbacks(new MyCallbacks(&this->_debug, this->_callback));
    _pOutputCharacteristic->addDescriptor(new BLE2904());
    // Add service UUID to advertising 
    _pServer->getAdvertising()->addServiceUUID(SERVICE_UUID);
    // Start the service
    _pService->start();
    // Start advertising
    _pServer->getAdvertising()->start();
    if (_debug) {
        Serial.println("BLE started");
    }
}
// Stop bluetooth
void BLEInterface::stopBLE() {
    if (_debug) {
        Serial.println("Stopping BLE");
    }
    _pServer->getAdvertising()->stop();
}