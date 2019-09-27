#include <Arduino.h>
#include "PumpInterface.h"
//************************************************************************************
// Public functions
//************************************************************************************
// Setup pinout
PumpInterface::PumpInterface(bool *pumpOn, uint8_t *tempDuration,
                             bool *tempActive, uint64_t *tempStart) {
    _pumpOn = pumpOn; // Set pointers to global values.
    _tempDuration = tempDuration;
    _tempActive = tempActive;
    _tempStart = tempStart;

    BOL = defaultBOL; // Set pinouts.
    ACT = defaultACT;
    ESC = defaultESC;
    UP = defaultUP;
    DOWN = defualtDOWN;
}
// Start pump interface, on default pinout
void PumpInterface::begin() {
    pinMode(BOL, OUTPUT); // Set the pins to output.
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
float PumpInterface::setTemp(float basalRate, uint8_t duration) {
    uint8_t step = 0;
    _tempBasal = basalRate;
    *_tempDuration = duration;
    if (*_tempActive) { // Check if temp is active - If it is, cancel it.
        if (!hasTempExpired()) {
            cancelTemp();
        }
        *_tempActive = false;
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
        float tempAboveOne = basalRate - 1.0;
        uint8_t tempStepBelowOne;
        uint8_t tempStepAboveOne;
        if (tempAboveOne > 0) { // Medtronic MMT-554 uses 0.025 incremnt below 1U/h and 0.05 above.
            tempStepBelowOne = 1 / tempBasalInterval;
            tempStepAboveOne = tempAboveOne / tempBasalIntervalAbove;
        } else {
            tempStepBelowOne = basalRate / tempBasalInterval;
            tempStepAboveOne = 0;
        }
        for (uint8_t i = 0; i < tempStepBelowOne; i++) {
            pressUP();
            step++; // Count number of presses.
        }
        for (uint8_t i = 0; i < tempStepAboveOne; i++) {
            pressUP();
            step++; // Count number of presses.
            if (step >= maxTempSteps) { // Make sure step is within bonuds.
                break;
            }
        }
    }
    if (basalRate > 3.55 && step == 91 && basalRate < 3.65) { // ????????????????????????
        pressUP();
        step++; // Count number of presses.
    }
    pressACT();
    escToMain();
    *_tempActive = true;
    *_tempStart = millis();
    if (step > 40) {
        return (float)(1 + ((step - 40) * tempBasalIntervalAbove));
    } else {
        return (float)(step * tempBasalInterval);
    }
}
// Cancel temp basal rate
void PumpInterface::cancelTemp() {
    if (*_tempActive) {
        if (hasTempExpired()) { // Check if temp has expired (Temp time has run out).
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
        *_tempActive = false;
    }
}
// Deliver bolus
float PumpInterface::setBolus(float amount) {
    uint8_t step = 0;
    for (uint8_t i = 0; i < 2; i++) {
        pressACT();
    }
    pressDOWN();
    pressACT();
    uint8_t bolusSteps = amount / bolusStepInterval;
    for (uint8_t i = 0; i < bolusSteps; i++) {
        pressUP();
        step++; // Count number of presses.
        if (step >= maxBolusSteps) { // Make sure step is within bonuds.
            break;
        }
    }
    pressACT();

    Serial.print("At bolus, pressed UP ");
    Serial.print(step);
    Serial.println(" times.");

    return (float)(step * bolusStepInterval);
}
// Stop pump
bool PumpInterface::stopPump() {
    if (*_pumpOn) {
        // Not yet implemented.




        *_pumpOn = false;
        return true;
    } else {
        return false;
    }
}
// Start pump
bool PumpInterface::startPump() {
    if (!*_pumpOn) {
        // Not yet implemented.



        *_pumpOn = true;
        return true;
    } else {
        return false;
    }
}
// Debug hardware pinout
bool PumpInterface::debug_hardware(char action) { // Debug function for testing hardware connectivity.
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
    //delay(press_delay);
    digitalWrite(BOL, HIGH);
    delay(press_time);
    digitalWrite(BOL, LOW);
    delay(press_delay);
}
// Press ACT on pump
void PumpInterface::pressACT() {
    //delay(press_delay);
    digitalWrite(ACT, HIGH);
    delay(press_time);
    digitalWrite(ACT, LOW);
    delay(press_delay);
}
// Press ESC on pump
void PumpInterface::pressESC() {
    //delay(press_delay);
    digitalWrite(ESC, HIGH);
    delay(press_time*4);
    digitalWrite(ESC, LOW);
    delay(press_delay);
}
// Press UP on pump
void PumpInterface::pressUP() {
    //delay(press_delay);
    digitalWrite(UP, HIGH);
    delay(press_time);
    digitalWrite(UP, LOW);
    delay(press_delay);
}
// Press DOWN on pump
void PumpInterface::pressDOWN() {
    //delay(press_delay);
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
bool PumpInterface::hasTempExpired() {
    if ((millis() - *_tempStart) >= (*_tempDuration * min_to_ms)) {
            return true;
    }
    return false;
}