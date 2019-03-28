#include <Arduino.h>
#include "PumpInterface.h"
//************************************************************************************
// Public functions
//************************************************************************************
// Setup pinout
PumpInterface::PumpInterface() {
    pinMode(BACK, OUTPUT);
    pinMode(ACT, OUTPUT);
    pinMode(ESC, OUTPUT);
    pinMode(UP, OUTPUT);
    pinMode(DOWN, OUTPUT);
}
// Set a temp basal rate for a duration
void PumpInterface::setTemp(float basalRate, uint8_t duration) {
    pressACT();
    for (uint8_t i = 0; i < 4; i++) {
        pressDOWN();
    }
    for (uint8_t i = 0; i < 2; i++) {
        pressACT();
    }
    uint8_t durationSteps = duration / durationStepInterval;
    for (uint8_t i = 0; i < durationSteps; i++) {
        pressUP();
    }
    pressACT();
    uint8_t tempSteps = basalRate / tempBasalInterval;
    for (uint8_t i = 0; i < tempSteps; i++) {
        pressUP();
    }
    pressACT();
    for (uint8_t i = 0; i < 2; i++) {
        pressESC();
    }
}
// Cancel temp basal rate
void PumpInterface::cancelTemp() {
    pressACT();
    for (uint8_t i = 0; i < 4; i++) {
        pressDOWN();
    }
    pressACT();
    pressDOWN();
    pressACT();
    for (uint8_t i = 0; i < 2; i++) {
        pressESC();
    }
}
// Deliver bolus
void PumpInterface::setBolus(float amount) {
    for (uint8_t i = 0; i < 2; i++) {
        pressACT();
    }
    pressDOWN();
    pressACT();
    uint8_t bolusSteps = amount / bolusStepInterval;
    for (uint8_t i = 0; i < bolusSteps; i++) {
        pressUP();
    }
    pressACT();
}
// Stop pump
bool PumpInterface::stopPump() {
    if (this->pumpOn) {





        this->pumpOn = false;
        return true;
    } else {
        return false;
    }
}
// Start pump
bool PumpInterface::startPump() {
    if (!this->pumpOn) {




        this->pumpOn = true;
        return true;
    } else {
        return false;
    }
}
//************************************************************************************
// Private functions
//************************************************************************************
// Press BACK on pump
void PumpInterface::pressBACK() {
    digitalWrite(BACK, HIGH);
    delay(press_time);
    digitalWrite(BACK, LOW);
    delay(press_delay);
}
// Press ACT on pump
void PumpInterface::pressACT() {
    digitalWrite(ACT, HIGH);
    delay(press_time);
    digitalWrite(ACT, LOW);
    delay(press_delay);
}
// Press ESC on pump
void PumpInterface::pressESC() {
    digitalWrite(ESC, HIGH);
    delay(press_time);
    digitalWrite(ESC, LOW);
    delay(press_delay);
}
// Press UP on pump
void PumpInterface::pressUP() {
    digitalWrite(UP, HIGH);
    delay(press_time);
    digitalWrite(UP, LOW);
    delay(press_delay);
}
// Press DOWN on pump
void PumpInterface::pressDOWN() {
    digitalWrite(DOWN, HIGH);
    delay(press_time);
    digitalWrite(DOWN, LOW);
    delay(press_delay);
}



// Print debug info
void PumpInterface::debug(uint8_t debuglvl, char character = null) {
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