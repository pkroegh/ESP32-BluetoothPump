#include <Arduino.h>
#include "MedtronicAPS.h"
#include "BluetoothInterface.h"
#include "PumpInterface.h"
//************************************************************************************
// Public functions
//************************************************************************************
// Start AndroidAPS communication
MedtronicAPS::MedtronicAPS() {
    this->wakeTime = millis();
    pump.begin(); // BACK, ACT, ESC, UP, DOWN
    MedBlue.begin(deviceName);
}
// Start AndroidAPS communication, with debug
MedtronicAPS::MedtronicAPS(bool debug) {
    this->_debug = debug;
    this->wakeTime = millis();
    pump.begin(); // BACK, ACT, ESC, UP, DOWN
    MedBlue.begin(deviceName,debug);
}
// Maintain connection with AndroidAPS
void MedtronicAPS::maintainConnection() {
    this->currentMillis = millis();
    handshake();
    pump.updateTime(this->currentMillis);
    processMessage(MedBlue.getMessage());
}
//************************************************************************************
// Private functions
//************************************************************************************
// Handshake with AndroidAPS
void MedtronicAPS::handshake() {
    if (!this->handshakingCompleted && ((this->currentMillis - this->lastMessageTime) 
        >= handshakeInterval)) {
        if (this->_debug) { Serial.println("Handshaking!..."); }
        sendWake();
        this->lastMessageTime = millis();
    }
    if (this->handshakingCompleted && (this->currentMillis - this->wakeTime) 
        >= (this->wakeInterval * min_to_ms * resetTimeScaler)) { //AndroidAPS didn't connect, reset
        this->handshakingCompleted = false;
        if (this->_debug) { Serial.println("Resetting handshake"); }
    }
}
// Process message from bluetooth
void MedtronicAPS::processMessage(String command) {
    if (command == "") { return; }
    if (this->_debug) { Serial.print("Got BT string: "); Serial.println(command); }
    if (command.indexOf(APS_ping) >= 0) {
        this->handshakingCompleted = true;
        gotPing();
    } else if (command.indexOf(APS_temp) >= 0) {
        updateTemp(command);
    } else if (command.indexOf(APS_wake) >= 0) {
        updateWakeTimer(command);
    } else if (command.indexOf(APS_sleep) >= 0) {
        sleepNow();
    }
}
// Ping message, send battery status
void MedtronicAPS::gotPing() {



    MedBlue.sendMessage("e=100");
    if (this->_debug) { Serial.println("Done handshaking"); }
}
// Temp command
void MedtronicAPS::updateTemp(String command) {
    if (command.indexOf("null") >= 0) {
        cancelTempBasal();
    } else {
        newTempBasal(command);
    }
}
// Isolate temp basal rate and duration
void MedtronicAPS::newTempBasal(String command) {
    if (command.length() > 2) {
        float basalRate = cutVariableFromString(command, APS_temp, 4, vFloat);
        uint8_t duration = cutVariableFromString(command, comm_variable1, 3, vInt);
        if (this->_debug) { newTempDebug(basalRate, duration); }
        pump.setTemp(basalRate, duration);
    }
    command = ESP_temp;
    if (!tempActive) { command.concat("null"); } 
    else {
        command.concat(tempBasal);
        command.concat(comm_variable1);
        command.concat(tempDuration);
    }
    MedBlue.sendMessage(command);
}
// Return to basal rate
void MedtronicAPS::cancelTempBasal() {
    pump.cancelTemp();
}
// Update timer
void MedtronicAPS::updateWakeTimer(String command) {
    this->wakeInterval = cutVariableFromString(command, APS_wake, 1, vInt);
    sendWake();
}
// Send wake message
void MedtronicAPS::sendWake() {
    String command = ESP_wake;
    command.concat(this->wakeInterval);
    MedBlue.sendMessage(command);
}
// Send sleep message and go to sleep
void MedtronicAPS::sleepNow() {
    if (this->_debug) { sleepNowDebug(this->wakeInterval); }
    MedBlue.sendMessage(ESP_sleep);
    esp_sleep_enable_timer_wakeup(this->wakeInterval * M_TO_uS_FACTOR);
    esp_deep_sleep_start();
}
// Isolate value from string
float MedtronicAPS::cutVariableFromString(String inputString, String leadingString, 
                                          int sizeOfVariable, int type) {
    //Example string: "getBasalRate rate: 0.9 duration: 30"
    //Isolate 0.9
    inputString.remove(0, inputString.indexOf(leadingString) + leadingString.length());
    //Example string: "0.9 duration: 30"
    if (inputString.length() != sizeOfVariable) {
        inputString.remove(sizeOfVariable, inputString.length());
    }
    if (type == 0) {
        return inputString.toInt();
    } else if (type == 1) {
        return inputString.toFloat();
    }
}
// Temp serial debug
void MedtronicAPS::newTempDebug(float basalRate, uint8_t duration) {
    Serial.println("Temporary basal rate update recieved!");
    Serial.print("TempBasal to be set: ");
    Serial.print(basalRate);
    Serial.print("U/h for tempDuration: ");
    Serial.print(duration);
    Serial.println(" min");
}
// Sleep serial debug
void MedtronicAPS::sleepNowDebug(uint8_t duration) {
    Serial.println("Going to sleep!");
    Serial.print("Waking in: ");
    Serial.print(duration);
    Serial.println(" min");
}