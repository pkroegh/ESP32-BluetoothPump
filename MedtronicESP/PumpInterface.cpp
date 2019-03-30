#include <Arduino.h>
#include "PumpInterface.h"
//************************************************************************************
// Public functions
//************************************************************************************
// Setup pinout
PumpInterface::PumpInterface() {
    this->BACK = defaultBACK;
    this->ACT = defaultACT;
    this->ESC = defaultESC;
    this->UP = defaultUP;
    this->DOWN = defualtDOWN;
}
// Start pump interface, on default pinout
void PumpInterface::begin() {
    pinMode(this->BACK, OUTPUT);
    pinMode(this->ACT, OUTPUT);
    pinMode(this->ESC, OUTPUT);
    pinMode(this->UP, OUTPUT);
    pinMode(this->DOWN, OUTPUT);
}
// Start pump interface, with defined pins
void PumpInterface::begin(uint8_t BACKpin, uint8_t ACTpin, uint8_t ESCpin, 
                     uint8_t UPpin, uint8_t DOWNpin) {
    this->BACK = BACKpin;
    this->ACT = ACTpin;
    this->ESC = ESCpin;
    this->UP = UPpin;
    this->DOWN = DOWNpin;

    pinMode(this->BACK, OUTPUT);
    pinMode(this->ACT, OUTPUT);
    pinMode(this->ESC, OUTPUT);
    pinMode(this->UP, OUTPUT);
    pinMode(this->DOWN, OUTPUT);
}
// Set a temp basal rate for a duration
void PumpInterface::setTemp(float basalRate, uint8_t duration) {
    tempBasal = basalRate;
    tempDuration = duration;
    if (tempActive) {
        cancelTemp();
        tempActive = false;
    }
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
    if (basalRate < tempBasalInterval) {
        pressDOWN();
    } else {
        uint8_t tempSteps = basalRate / tempBasalInterval;
        for (uint8_t i = 0; i < tempSteps; i++) {
            pressUP();
        }
    }
    pressACT();
    for (uint8_t i = 0; i < 2; i++) {
        pressESC();
    }
    tempActive = true;
    tempStart = millis();
}
// Cancel temp basal rate
void PumpInterface::cancelTemp() {
    if (!tempActive) {
        return;
    }
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
    tempActive = false;
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
    if (pumpOn) {





        pumpOn = false;
        return true;
    } else {
        return false;
    }
}
// Start pump
bool PumpInterface::startPump() {
    if (!pumpOn) {




        pumpOn = true;
        return true;
    } else {
        return false;
    }
}
// Compare temp start temp duration, and set temp active false, if temp over
void PumpInterface::updateTime(uint64_t currentMillis) {
    if ((currentMillis - tempStart) >= (tempDuration * min_to_ms)) {
        tempActive = false;
    }
}
//************************************************************************************
// Private functions
//************************************************************************************
// Press BACK on pump
void PumpInterface::pressBACK() {
    digitalWrite(this->BACK, HIGH);
    delay(press_time);
    digitalWrite(this->BACK, LOW);
    delay(press_delay);
}
// Press ACT on pump
void PumpInterface::pressACT() {
    digitalWrite(this->ACT, HIGH);
    delay(press_time);
    digitalWrite(this->ACT, LOW);
    delay(press_delay);
}
// Press ESC on pump
void PumpInterface::pressESC() {
    digitalWrite(this->ESC, HIGH);
    delay(press_time);
    digitalWrite(this->ESC, LOW);
    delay(press_delay);
}
// Press UP on pump
void PumpInterface::pressUP() {
    digitalWrite(this->UP, HIGH);
    delay(press_time);
    digitalWrite(this->UP, LOW);
    delay(press_delay);
}
// Press DOWN on pump
void PumpInterface::pressDOWN() {
    digitalWrite(this->DOWN, HIGH);
    delay(press_time);
    digitalWrite(this->DOWN, LOW);
    delay(press_delay);
}