#include <Arduino.h>
#include "PumpInterface.h"
//************************************************************************************
// Public functions
//************************************************************************************
// Setup pinout
PumpInterface::PumpInterface() {
    BOL = defaultBOL;
    ACT = defaultACT;
    ESC = defaultESC;
    UP = defaultUP;
    DOWN = defualtDOWN;
}
// Start pump interface, on default pinout
void PumpInterface::begin() {
    pinMode(BOL, OUTPUT);
    pinMode(ACT, OUTPUT);
    pinMode(ESC, OUTPUT);
    pinMode(UP, OUTPUT);
    pinMode(DOWN, OUTPUT);
}
// Start pump interface, with defined pins
void PumpInterface::begin(uint8_t BOLpin, uint8_t ACTpin, uint8_t ESCpin, 
                     uint8_t UPpin, uint8_t DOWNpin) {
    BOL = BOLpin;
    ACT = ACTpin;
    ESC = ESCpin;
    UP = UPpin;
    DOWN = DOWNpin;

    pinMode(BOL, OUTPUT);
    pinMode(ACT, OUTPUT);
    pinMode(ESC, OUTPUT);
    pinMode(UP, OUTPUT);
    pinMode(DOWN, OUTPUT);
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
    for (uint8_t i = 0; i < 3; i++) {
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
        pressUP();
        pressDOWN();
    } else {
        uint8_t tempSteps = basalRate / tempBasalInterval;
        for (uint8_t i = 0; i < tempSteps; i++) {
            pressUP();
        }
    }
    pressACT();
    escToMain();
    tempActive = true;
    tempStart = millis();
}
// Cancel temp basal rate
void PumpInterface::cancelTemp() {
    if (!tempActive) {
        return;
    }
    pressACT();
    for (uint8_t i = 0; i < 3; i++) {
        pressDOWN();
    }
    pressACT();
    pressDOWN();
    pressACT();
    escToMain();
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
// Debug hardware pinout
bool PumpInterface::debug_hardware(char action) {
    switch (action) {
        case 'b': pressBOL(); return true; break;
        case 'a': pressACT(); return true; break;
        case 'e': pressESC(); return true; break;
        case 'u': pressUP(); return true; break;
        case 'd': pressDOWN(); return true; break;
        default: return false; break;
    }
}
//************************************************************************************
// Private functions
//************************************************************************************
// Press BOL on pump
void PumpInterface::pressBOL() {
    delay(press_delay);
    digitalWrite(BOL, HIGH);
    delay(press_time);
    digitalWrite(BOL, LOW);
    delay(press_delay);
}
// Press ACT on pump
void PumpInterface::pressACT() {
    delay(press_delay);
    digitalWrite(ACT, HIGH);
    delay(press_time);
    digitalWrite(ACT, LOW);
    delay(press_delay);
}
// Press ESC on pump
void PumpInterface::pressESC() {
    delay(press_delay);
    digitalWrite(ESC, HIGH);
    delay(press_time*4);
    digitalWrite(ESC, LOW);
    delay(press_delay);
}
// Press UP on pump
void PumpInterface::pressUP() {
    delay(press_delay);
    digitalWrite(UP, HIGH);
    delay(press_time);
    digitalWrite(UP, LOW);
    delay(press_delay);
}
// Press DOWN on pump
void PumpInterface::pressDOWN() {
    delay(press_delay);
    digitalWrite(DOWN, HIGH);
    delay(press_time);
    digitalWrite(DOWN, LOW);
    delay(press_delay);
}
// Escape from basal menu to main menu
void PumpInterface::escToMain() {
    delay(press_delay*4);
    for (uint8_t i = 0; i < 2; i++) {
        pressESC();
    }
}