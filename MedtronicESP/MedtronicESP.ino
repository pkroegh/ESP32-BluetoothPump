//************************************************************************************
// Librarys
#include <Arduino.h>
#include "BLEInterface.h"
#include "PumpInterface.h"
//************************************************************************************
#define doDebug true
#ifdef doDebug
    #define serialBaud 115200
#endif

#define M_TO_uS_FACTOR 60000000
#define uS_TO_S_FACTOR 1000000
#define min_to_ms 60000

#define cooldownTime 1

bool sleepConfirmed = false;

// Password for accepting commands
String password = "123456"; // Device password
// Bluetooth device name
String deviceName = "MedESP"; // Device name
//************************************************************************************
// RTC data variables (Persistent variables)
RTC_DATA_ATTR bool pumpOn = true;
RTC_DATA_ATTR float tempBasal; // Temp basal rate, in U/h
RTC_DATA_ATTR uint8_t tempDuration; // Duration of temp basal, in min.
RTC_DATA_ATTR bool tempActive = false;
RTC_DATA_ATTR uint64_t tempStart; // Temp start time
float bolusSet = 0;
//************************************************************************************
// Library instance initialization
BLEInterface ble(&deviceName, &messageHandler);
PumpInterface pump(&pumpOn, &tempDuration, &tempActive, &tempStart);
//************************************************************************************
// Setup
void setup() {
    #ifdef doDebug
        Serial.begin(serialBaud);
    #endif
    // Start AndroidAPS communication
    ble.begin(doDebug);
    pump.begin(15,14,32,27,33); // BOL, ACT, ESC, UP, DOWN
}
//************************************************************************************
// Main loop
void loop() {

}
//************************************************************************************
// New message callback for BLEInterface
void messageHandler(String message) {
    if (message.indexOf(password) == 0) {
        if (doDebug) {
            Serial.println("Correct password!");
        }
        message.remove(0,password.length() + 1);
        Serial.print("Message is: ");
        Serial.println(message);
    } else {
        gotWrongPassword();
    }
    char action = message.charAt(0);
    Serial.print("At action: ");
    Serial.println(action);
    switch (action) {
        case ble.pingAPS:
            gotPing();
        break;
        case ble.tempAPS:
            gotTemp(message);
        break;
        case ble.bolusAPS:
            gotBolus(message);
        break;
        case ble.sleepAPS:
            gotSleep(message);
        break;
    }
}
// Ping message, send battery status
void gotPing() {

    ble.sendBattery(100);
}
// Deliver bolus
void gotBolus(String command) {
    float bolus = getFloatfromInsideStr(command, String(ble.bolusAPS) + "=", String(ble.endAPS));
    if (bolusSet != bolus) {
        bolusSet = bolus;
        #ifdef doDebug
            Serial.println("Setting bolus.");
        #endif
        pump.setBolus(bolus);
    } else {
        #ifdef doDebug
            Serial.println("Bolus already set.");
        #endif
    }
    ble.sendBolus(bolus);
}
// Temp command
void gotTemp(String command) {
    float basalRate = getFloatfromInsideStr(command, String(ble.tempAPS) + "=", String(ble.endAPS));
    uint8_t duration = getIntfromInsideStr(command, String(ble.comm_variable) + "=", String(ble.endAPS));
    if (tempBasal != basalRate) {
        tempBasal = basalRate;
        if (tempBasal <= 0.0) {
            #ifdef doDebug
                Serial.println("Canceling temp.");
            #endif
            pump.cancelTemp();
        } else {
            #ifdef doDebug
                Serial.println("Setting new temp.");
            #endif
            pump.setTemp(basalRate, duration);
        } 
    } else {
        #ifdef doDebug
            Serial.println("Temp already set.");
        #endif
    }
    ble.sendTemp(basalRate, duration);
}
// Send sleep message and go to sleep
void gotSleep(String command) {
    uint8_t wakeInterval = getIntfromInsideStr(command, String(ble.sleepAPS) + "=", String(ble.endAPS));
    ble.sendSleep(wakeInterval);
    #ifdef doDebug
        sleepNowDebug(wakeInterval); 
    #endif
    if (sleepConfirmed) {
        sleepESP(wakeInterval);
    } else {
        sleepConfirmed = true;
    }
}
// Put ESP to sleep
void sleepESP(uint8_t sleepTime) {
    ble.end();
    esp_sleep_enable_timer_wakeup(sleepTime * M_TO_uS_FACTOR);
    esp_deep_sleep_start();
}

void gotWrongPassword() {
    ble.end();
    sleepESP(cooldownTime);
}
/*
// Isolate value from string, as int (32bit)
int32_t getIntfromStr(String inputString, String leadingString, 
                        uint8_t sizeOfVariable) {
    //Example string: "getBasalRate rate: 0.9 duration: 30"
    //Isolate 0.9
    inputString.remove(0, inputString.indexOf(leadingString) + leadingString.length());
    //Example string: "0.9 duration: 30"
    if (inputString.length() != sizeOfVariable) {
        inputString.remove(sizeOfVariable, inputString.length());
    }
    return inputString.toInt();
}
// Isolate value from string, as float
float getFloatfromStr(String inputString, String leadingString, 
                        uint8_t sizeOfVariable) {
    inputString.remove(0, inputString.indexOf(leadingString) + leadingString.length());
    if (inputString.length() != sizeOfVariable) {
        inputString.remove(sizeOfVariable, inputString.length());
    }
    return inputString.toFloat();
}
*/
// Isolate value from indside string, as int (32bit)
int32_t getIntfromInsideStr(String inputString, String leadingString, 
                        String followingString) {
    inputString.remove(0, inputString.indexOf(leadingString) + leadingString.length());
    inputString.remove(inputString.indexOf(followingString), inputString.length());
    return inputString.toInt();
}
// Isolate a value from indside a string, as float
float getFloatfromInsideStr(String inputString, String leadingString, 
                        String followingString) {
    inputString.remove(0, inputString.indexOf(leadingString) + leadingString.length());
    inputString.remove(inputString.indexOf(followingString), inputString.length());
    return inputString.toFloat();
}
#ifdef doDebug
    /*
    // Temp serial debug
    void newTempDebug(float basalRate, uint8_t duration) {
        Serial.println("Temporary basal rate update recieved!");
        Serial.print("TempBasal to be set: ");
        Serial.print(basalRate);
        Serial.print("U/h for tempDuration: ");
        Serial.print(duration);
        Serial.println(" min");
    }
    */
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