//Librarys
//------------------------------------------------------------------------------------
#include <math.h>
#include "MedtronicBluetooth.h"
#include "PumpInterface.h"


#include "debug.h"

//************************************************************************************
//Bluetooth device name
//------------------------------------------------------------------------------------
const String deviceName = "MedtronicESP"; //Defines the name of the device
//************************************************************************************
//RTC data variables (Persistent variables)
//------------------------------------------------------------------------------------
RTC_DATA_ATTR float tempBasal; //Temp basal rate, in U/h
RTC_DATA_ATTR uint8_t tempDuration; //Duration of temp basal, in min.
RTC_DATA_ATTR bool tempActive = false;
//************************************************************************************
//Temp variables
//------------------------------------------------------------------------------------
uint8_t wakeInterval = 1;
#define handshakeInterval 3000 //Milliseconds between handshake attempt3
#define resetTimeScaler 2
bool handshakingCompleted = false;
uint64_t lastMessageTime;
uint64_t wakeTime;
uint64_t currentMillis;

#define M_TO_uS_FACTOR 60000000
#define uS_TO_S_FACTOR 1000000
#define min_to_ms 60000

#define vInt 0
#define vFloat 1
#define pOFF 0
#define pON 1

#define ESP_battery "e="
#define ESP_wake "w="
#define ESP_temp "t="
#define ESP_sleep "s"

#define APS_ping "P"
#define APS_temp "T="
#define APS_wake "W="
#define APS_sleep "S"
#define comm_variable1 ":0="

#define debug_serial
#define ignore_confirm
#ifdef debug_serial
#define print_setup
#define print_bluetooth
#endif

//************************************************************************************
//Library instance initialization
//------------------------------------------------------------------------------------
MedtronicBluetooth MedBlue(deviceName, true);
PumpInterface pump();
//************************************************************************************
//Process string
void processMessage(String command) {
    if (command == null) {
        return;
    }
    #ifdef debug_serial
        Serial.print("Got BT string: ");
        Serial.println(command);
    #endif
    if (command.indexOf(APS_ping) >= 0) {
        handshakingCompleted = true;
        MedBlue.sendMessage("e=100");
        #ifdef debug_serial
            Serial.println("Done handshaking");
        #endif
    } else if (command.indexOf(APS_temp) >= 0) {
        if (command.indexOf("null") >= 0) {
            cancelTempBasal();
        } else {
            newTempBasal(command);
        }
    } else if (command.indexOf(APS_wake) >= 0) {
        updateWakeTimer(command);
    } else if (command.indexOf(APS_sleep) >= 0) {
        sleepNow();
    }
}
//************************************************************************************
//Isolate value from string
float cutVariableFromString(String inputString, String leadingString, int sizeOfVariable, int type) {
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
//************************************************************************************
//Isolate temp basal rate and duration
void newTempBasal(String command) {
    if (command.length() > 2) {
        tempBasal = cutVariableFromString(command, APS_temp, 4, vFloat);
        tempDuration = cutVariableFromString(command, comm_variable1, 3, vInt);
        #ifdef debug_serial
            Serial.println("Temporary basal rate update recieved!");
            Serial.print("TempBasal to be set: ");
            Serial.print(tempBasal);
            Serial.print("U/h for tempDuration: ");
            Serial.print(tempDuration);
            Serial.println(" min");
        #endif
        if (tempActive) {
            pump.cancelTemp();
        }
        pump.setTemp(tempBasal, tempDuration);
        tempActive = true;
    }
    command = ESP_temp;
    if (!tempActive) {
        command.concat("null");
    } else {
        command.concat(tempBasal);
        command.concat(comm_variable1);
        command.concat(tempDuration);
    }
    MedBlue.sendMessage(command);
}
//************************************************************************************
//Return to basal rate
void cancelTempBasal() {
    if (tempActive) {
        pump.cancelTemp();
        tempActive = false;
    }
}
//************************************************************************************
//Update timer
void updateWakeTimer(String command) {
    wakeInterval = cutVariableFromString(command, APS_wake, 1, vInt);
    sendWake();
}
//************************************************************************************
void sendWake() {
    String command = ESP_wake;
    command.concat(wakeInterval);
    MedBlue.sendMessage(command);
}
//************************************************************************************
void sleepNow() {
    #ifdef debug_serial
        Serial.println("Going to sleep!");
        Serial.print("Waking in: ");
        Serial.print(wakeInterval);
        Serial.println(" min");
    #endif
    MedBlue.sendMessage(ESP_sleep);
    esp_sleep_enable_timer_wakeup(wakeInterval * M_TO_uS_FACTOR);
    esp_deep_sleep_start();
}
//************************************************************************************
void handshake() {
    if (!handshakingCompleted && ((currentMillis - lastMessageTime) >= handshakeInterval)) {
        #ifdef debug_serial
            Serial.println("Handshaking!...");
        #endif
        sendWake();
        lastMessageTime = millis();
    }
    if (handshakingCompleted && (currentMillis - wakeTime) >= (wakeInterval * min_to_ms * resetTimeScaler)) { //AndroidAPS didn't connect, reset
        handshakingCompleted = false;
        #ifdef debug_serial
            Serial.println("Resetting handshake");
        #endif
    }
}
//************************************************************************************
//Setup
void setup() {
    #ifdef debug_serial
        Serial.begin(115200);
    #endif
    wakeTime = millis();
}
//************************************************************************************
//Main loop
void loop() {
    currentMillis = millis();
    handshake();
    processMessage(MedBlue.getMessage());
}
//************************************************************************************