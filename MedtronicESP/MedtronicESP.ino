// Librarys
//************************************************************************************
#include <Arduino.h>
#include "BluetoothInterface.h"
#include "PumpInterface.h"
#include "MakeLifeEasy.h"
//************************************************************************************
#define doDebug true
#ifdef doDebug
    #define serialBaud 115200
#endif

#define M_TO_uS_FACTOR 60000000
#define uS_TO_S_FACTOR 1000000
#define min_to_ms 60000

const String ESP_battery = "e=";
const String ESP_wake = "w=";
const String ESP_temp = "t=";
const String ESP_sleep = "s";

const String APS_ping = "P";
const String APS_temp = "T=";
const String APS_wake = "W=";
const String APS_sleep = "S";
const String comm_variable1 = ":0=";

#define handshakeInterval 5000 //Milliseconds between handshake attempt
#define resetTimeScaler 2

// Password for accepting commands
const String keyword = "dHyr7823Bh"; // Device password

// Bluetooth device name
const String deviceName = "MedtronicESP"; // Device name
uint8_t wakeInterval = 1;
bool handshakingCompleted = false;
uint64_t lastMessageTime;
uint64_t wakeTime;
uint64_t currentMillis;
//************************************************************************************
// RTC data variables (Persistent variables)
RTC_DATA_ATTR bool pumpOn = true;
RTC_DATA_ATTR float tempBasal; // Temp basal rate, in U/h
RTC_DATA_ATTR uint8_t tempDuration; // Duration of temp basal, in min.
RTC_DATA_ATTR bool tempActive = false;
RTC_DATA_ATTR uint64_t tempStart; // Temp start time
//************************************************************************************
// Library instance initialization
PumpInterface pump;
BluetoothInterface MedBlue;
//************************************************************************************
// Setup
void setup() {
    #ifdef doDebug
        Serial.begin(serialBaud);
    #endif
    wakeTime = millis();
    // Start AndroidAPS communication
    pump.begin(15,14,32,27,33); // BOL, ACT, ESC, UP, DOWN
    MedBlue.begin(deviceName, true);
}
//************************************************************************************
// Main loop
void loop() {
    currentMillis = millis();
    handshake();
    pump.updateTime(currentMillis);
    processMessage(MedBlue.getMessage());
    #ifdef doDebug
        serialAction();
    #endif
}
//************************************************************************************
// Handshake with AndroidAPS
void handshake() {
    if (!handshakingCompleted && ((currentMillis - lastMessageTime) 
        >= handshakeInterval)) {
        #ifdef doDebug
            Serial.println("Handshaking!..."); 
        #endif
        sendWake();
        lastMessageTime = millis();
    }
    if (handshakingCompleted && (currentMillis - wakeTime) 
        >= (wakeInterval * min_to_ms * resetTimeScaler)) { //AndroidAPS didn't connect, reset
        handshakingCompleted = false;
        #ifdef doDebug
            Serial.println("Resetting handshake");
        #endif
    }
}
// Process message from bluetooth
void processMessage(String command) {
    if (command == "") { return; }
    #ifdef doDebug
        Serial.print("Got BT string: "); 
        Serial.print(command);
    #endif
    if (command.indexOf(keyword) == 0) {
        #ifdef doDebug
            Serial.println(" - Correct password"); 
        #endif
        if (command.indexOf(APS_ping) >= 0) {
            handshakingCompleted = true;
            gotPing();
        } else if (command.indexOf(APS_temp) >= 0) {
            updateTemp(command);
        } else if (command.indexOf(APS_wake) >= 0) {
            updateWakeTimer(command);
        } else if (command.indexOf(APS_sleep) >= 0) {
            sleepNow();
        }
    } 
    #ifdef doDebug
        else {
            Serial.println(" - Wrong password");
        }
    #endif
}
// Ping message, send battery status
void gotPing() {



    MedBlue.sendMessage("e=100");
    #ifdef doDebug
        Serial.println("Done handshaking");
    #endif
}
// Temp command
void updateTemp(String command) {
    if (command.indexOf("null") >= 0) {
        cancelTempBasal();
    } else {
        newTempBasal(command);
    }
}
// Isolate temp basal rate and duration
void newTempBasal(String command) {
    if (command.length() > 2) {
        float basalRate = getFloatfromStr(command, APS_temp, 4);
        uint8_t duration = getIntfromStr(command, comm_variable1, 3);
        #ifdef doDebug
            newTempDebug(basalRate, duration);
        #endif
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
void cancelTempBasal() {
    pump.cancelTemp();
}
// Update timer
void updateWakeTimer(String command) {
    wakeInterval = getIntfromStr(command, APS_wake, 1);
    sendWake();
}
// Send wake message
void sendWake() {
    String command = ESP_wake;
    command.concat(wakeInterval);
    MedBlue.sendMessage(command);
}
// Send sleep message and go to sleep
void sleepNow() {
    MedBlue.sendMessage(ESP_sleep);
    delay(100); // Wait a short time to make sure message was sendt before shutting down
    #ifdef doDebug
        sleepNowDebug(wakeInterval); 
    #endif
    MedBlue.end();
    esp_sleep_enable_timer_wakeup(wakeInterval * M_TO_uS_FACTOR);
    esp_deep_sleep_start();
}
#ifdef doDebug
    // Temp serial debug
    void newTempDebug(float basalRate, uint8_t duration) {
        Serial.println("Temporary basal rate update recieved!");
        Serial.print("TempBasal to be set: ");
        Serial.print(basalRate);
        Serial.print("U/h for tempDuration: ");
        Serial.print(duration);
        Serial.println(" min");
    }
    // Sleep serial debug
    void sleepNowDebug(uint8_t duration) {
        Serial.println("Going to sleep!");
        Serial.print("Waking in: ");
        Serial.print(duration);
        Serial.println(" min");
    }
    // Debug pump hardware interface
    void serialAction() {
        if (Serial.available() > 0) {
            String action = Serial.readStringUntil('\r');
            Serial.print("Got Serial message: ");
            Serial.println(action);
            hardwareDebug(action);
            if (action.indexOf("tempBasal") >= 0) {
                Serial.println("Setting temp basal");
                pump.setTemp(1,30);
            } else if (action.indexOf("cancelTemp") >= 0) {
                Serial.println("Canceling temp basal");
                pump.cancelTemp();
            }
        }
    }
    // Hardware debug
    void hardwareDebug(String action) {
        if (action.indexOf("hwDebug:") >= 0) {
            uint8_t index = action.indexOf(":");
            char button = action[index+1];
            Serial.print("Pressing ");
            Serial.print(button);
            Serial.print(" on pump - ");
            if (pump.debug_hardware(button)) {
                Serial.println("Success");
            } else { 
                Serial.println("Failed");
            }
        }
    }
#endif