#include <Arduino.h>
#include "PumpInterface.h"
//************************************************************************************
// Public functions
//************************************************************************************
// Setup pinout
PumpInterface::PumpInterface(bool *pumpOn, uint8_t *tempDuration,
                             bool *tempActive, uint32_t *tempStart, 
                             uint32_t *timeSinceRun) {
    _pumpOn = pumpOn; // Set pointers to global values.
    _tempDuration = tempDuration;
    _tempActive = tempActive;
    _tempStart = tempStart;
    _timeSinceRun = timeSinceRun;

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
    cancelTemp();
    uint8_t step = 0;
    _tempBasal = basalRate;
    *_tempDuration = duration;
    pressACT(); 
    for (uint8_t i = 0; i < 3; i++) {
        pressDOWN();
    }
    for (uint8_t i = 0; i < 2; i++) {
        pressACT();
    }
    uint8_t durationSteps = duration / durationStepInterval;
    if (durationSteps == 0) { // Attempted to set temp with duration 0 - Stop.
        escToMain(); // Make sure pump is in main menu
        return -2.0;
    }
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
    *_tempStart = timeNow(_timeSinceRun);
    if (step > 40) {
        return (float)(1 + ((step - 40) * tempBasalIntervalAbove));
    } else {
        return (float)(step * tempBasalInterval);
    }
}
// Cancel temp basal rate
bool PumpInterface::cancelTemp() {
    escToMain(); // Make sure pump is in main menu
    if (*_tempActive) {
        if (hasTempExpired()) { // Check if temp has expired (Temp time has run out).
            Serial.println("Temp has expired");
            return false;
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
        return true;
    }
}
// Deliver bolus
/*peter23/10
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
//peter23/10
*/ 
// Debug hardware pinout
bool PumpInterface::debug_hardware(char action) { // Debug function for testing hardware connectivity.
    switch (action) {
        //case 'b': pressBOL(); return true; break;
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
/*peter23/10
void PumpInterface::pressBOL() {
    //delay(press_delay);
    digitalWrite(BOL, HIGH);
    delay(press_time);
    digitalWrite(BOL, LOW);
    delay(press_delay);
}
//peter23/10
*/
// Press ACT on pump
void PumpInterface::pressACT() {
    //delay(press_delay);
    digitalWrite(ACT, HIGH);
    delay(esc_time);
    digitalWrite(ACT, LOW);
    delay(esc_delay);
}
// Press ESC on pump
void PumpInterface::pressESC() {
    //delay(press_delay);
    digitalWrite(ESC, HIGH);
    delay(esc_time);
    digitalWrite(ESC, LOW);
    delay(esc_delay);
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
    delay(esc_time);
    for (uint8_t i = 0; i < 4; i++) {
        pressESC();
    }
}
bool PumpInterface::hasTempExpired() {
    if ((timeNow(_timeSinceRun) - *_tempStart) > getMillisFromDuration(*_tempDuration)) {
        return true;
    } else {
        return false;
    }
}
// Convert duratin in min to millis.
uint32_t PumpInterface::getMillisFromDuration(uint8_t duration) {
    switch (duration) {
        case 0:
            return 0;
        break;
        case 30:
            return min30Millis;
        break;
        case 60:
            return min60Millis;
        break;
        case 90:
            return min90Millis;
        break;
        case 120:
            return min120Millis;
        break;
        default:
            return min30Millis;
        break;
    }
}
