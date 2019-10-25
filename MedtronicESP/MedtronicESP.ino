//20191020***23/10peter *********************************************************************************
// Librarys
#include <Arduino.h>
#include "NoSleepyTimer.h"
#include "BLEInterface.h"
#include "PumpInterface.h"
//************************************************************************************
#define doDebug true // Set to true to enable serial debug.
#ifdef doDebug
    #define serialBaud 115200 // Serial baud rate for debug.
#endif
#define M_TO_uS_FACTOR 60000000 // Minuts to microseconds factor.
#define uS_TO_S_FACTOR 1000000 // Micro seconds to seconds factor.
#define min_to_ms 60000 // Min to milliseconds factor.
#define cooldownTime 1 // Minuts to sleep on recieving a command with wrong password.
bool sleepConfirmed = false; // Variable to check if sleep is confirmed by AndroidAPS. Fixes a bug where ESP doesn't sleep after getting sleep command.
// Bluetooth device name
String deviceName = "MedESP"; // Device name, this is used in AndroidAPS to identify the bluetooth device. If changed here, it needs to be changed in AndroidAPS before compiling and installing AndroidAPS.
//************************************************************************************
// User settable variables!
// Password for accepting commands (SHOULD BE CHANGE BEFORE USE!)
String password = "123456"; // Device password, is used to ensure that it is only AndroidAPS that can set commands. This should be changed here, and then set in the plugin settings menu in AndroidAPS.
//************************************************************************************
// RTC data variables (Persistent variables)
RTC_DATA_ATTR bool pumpOn = true; // Pump status, set to off when basal delivery is stopped.
RTC_DATA_ATTR float tempBasal = -1; // Temp basal rate, in U/h
RTC_DATA_ATTR uint8_t tempDuration = 0; // Duration of temp basal, in min.
RTC_DATA_ATTR bool tempActive = false; // Temp basal status, true when a temp basal is set.
RTC_DATA_ATTR uint32_t tempStart = 0; // Temp start time
RTC_DATA_ATTR uint32_t timeSinceRun = 0; // Millis is reset on each sleep, store systemtime in this value.
float bolusSet = 0; // Amount of bolus to set.
//************************************************************************************
// Library instance initialization
BLEInterface ble(&deviceName, &messageHandler); // Instance initializer for ble interface.
PumpInterface pump(&pumpOn, &tempDuration, &tempActive, &tempStart, &timeSinceRun); // Instance initializer for physical hardware interface.
//************************************************************************************
// Setup
void setup() {
    #ifdef doDebug
        Serial.begin(serialBaud); // Starts serial communication for debug.
    #endif
    // Start AndroidAPS communication
    ble.begin(doDebug); // Starts ble.
    pump.begin(15,14,32,27,33); // Starts and sets pinout for pump buttons. BOL, ACT, ESC, UP, DOWN
    // To change output pins to pump button relays, change in above line.
}
//************************************************************************************
// Main loop
void loop() {
  serialAction();
}
//************************************************************************************
// New message callback for BLEInterface
void messageHandler(String message) {
    if (message.indexOf(password) == 0) { // Check if password is correct.
        if (doDebug) {
            Serial.println("Correct password!");
        }
        message.remove(0,password.length() + 1); // Remove password from message string.
        if (doDebug) {
            Serial.print("Message is: ");
            Serial.println(message); // Print message string.
        }
    } else { // Sleep the ESP on wrong password.
        sleepESP(cooldownTime); // Sleep the ESP
    }
    char action = message.charAt(0); // Get the char at the first position of the message. This will be the command to be set.
    Serial.print("At action: ");
    Serial.println(action);
    switch (action) { // Check what action is to be set.
        case ble.pingAPS: // Ping action -> Send battery status.
            gotPing();
        break;
        case ble.tempAPS: // Temp action -> Set temp in pump.
            gotTemp(message);
        break;
        case ble.bolusAPS: // Bolus action -> Make pump deliver bolus.
            gotBolus(message);
        break;
        case ble.sleepAPS: // Sleep action -> Put ESP32 to sleep.
            gotSleep(message);
        break;
    }
}
// Ping message, send battery status
void gotPing() {
    // Not yet implemented. Should check voltage level of a connected battery and send battery status.
    ble.sendBattery(100);
}
// Deliver bolus
void gotBolus(String command) {
    float setBolus = 0;
    // Cut the bolus value from the message string.
    float bolus = getFloatfromInsideStr(command, String(ble.bolusAPS) + "=", String(ble.endAPS));
    /*
    if (bolusSet != bolus) { // Check if bolus is already delivered - Should always be false, prevents to bolus deliveries on same wake cycle.
        bolusSet = bolus;
        #ifdef doDebug
            Serial.println("Setting bolus.");
        #endif
        setBolus = pump.setBolus(bolus); // Send bolus to pump interface.
        Serial.print("Delivered: ");
        Serial.print(setBolus);
        Serial.println(" U");
    } else {
        #ifdef doDebug
            Serial.println("Bolus already set.");
        #endif
    }
    */
    ble.sendBolus(setBolus); // Echo the command back to AndroidAPS to confirm the bolus has been set.
}
// Temp command
void gotTemp(String command) {
    float setBasalRate = 0;
    // Cut temp basal value from message string.
    float basalRate = getFloatfromInsideStr(command, String(ble.tempAPS) + "=", String(ble.comm_variable));
    // Cut temp duration from message string.
    uint8_t duration = getIntfromInsideStr(command, String(ble.comm_variable) + "=", String(ble.endAPS));
    if (tempBasal != basalRate) { // Check if temp is not already set. Will be false if temp was set in last wake cycle.
        if (basalRate < -0.5) { // Check if temp value is zero -> Pump should stop current temp delivery.
            #ifdef doDebug
                Serial.println("Canceling temp.");
            #endif
            bool confirmed = pump.cancelTemp(); // Cancel the current temp.
            if (confirmed) {
                setBasalRate = -1.0;
            } else {
                setBasalRate = -2.0;
            }
        } else {
            #ifdef doDebug
                Serial.println("Setting new temp.");
            #endif
            setBasalRate = pump.setTemp(basalRate, duration); // Set the new temp in the pump.
            Serial.print("Set basal to: ");
            Serial.print(setBasalRate);
            Serial.println(" U/h");
        } 
        tempBasal = basalRate;
    } else {
        setBasalRate = basalRate;
        #ifdef doDebug
            Serial.println("Temp already set.");
        #endif
    }
    ble.sendTemp(setBasalRate, duration); // Echo command back to confirm it is set.
}
// Send sleep message and go to sleep
void gotSleep(String command) {
    // Cut the number of minuts to sleep from the message string.
    uint8_t wakeInterval = getIntfromInsideStr(command, String(ble.sleepAPS) + "=", String(ble.endAPS)); 
    ble.sendSleep(wakeInterval); // Echo command back to AndroidAPS.
    #ifdef doDebug
        sleepNowDebug(wakeInterval); // Print sleep time.
    #endif
    if (sleepConfirmed) {
        sleepESP(wakeInterval); // Sleep the ESP.
    } else { 
        sleepConfirmed = true;
    }
    // AndroidAPS fails to read ble status if ESP sleeps as soon as it gets the sleep command. 
    // Therefore AndroidAPS sends the sleep command again when it gets the sleep conformation from the ESP.
}
// Put ESP to sleep
void sleepESP(uint8_t sleepTime) {
    ble.end(); // Stop the ble communication.
    timePrepareSleep(&timeSinceRun, &tempStart, sleepTime);
    esp_sleep_enable_timer_wakeup(sleepTime * M_TO_uS_FACTOR); // Set the timer to wake the ESP after the sleep.
    esp_deep_sleep_start(); // Sleep the ESP.
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
    // Find the character before desired value, remove everything up to the desired value.
    inputString.remove(0, inputString.indexOf(leadingString) + leadingString.length());
    // Find the character after the desired value, remove everything after the desired balue.
    inputString.remove(inputString.indexOf(followingString), inputString.length());
    return inputString.toInt(); // Return value as int.
}
// Isolate a value from indside a string, as float
float getFloatfromInsideStr(String inputString, String leadingString, 
                        String followingString) {
    // Find the character before desired value, remove everything up to the desired value.
    inputString.remove(0, inputString.indexOf(leadingString) + leadingString.length());
    // Find the character after the desired value, remove everything after the desired balue.
    inputString.remove(inputString.indexOf(followingString), inputString.length());
    int8_t comma = inputString.indexOf(",");
    if (comma >= 0) {
        inputString.setCharAt(comma, '.');
    }
    return inputString.toFloat(); // Return value as float.
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
            /*
            if (action.indexOf("tempBasal") >= 0) {
                Serial.println("Setting temp basal");
                pump.setTemp(1,30);
            } else if (action.indexOf("cancelTemp") >= 0) {
                Serial.println("Canceling temp basal");
                pump.cancelTemp();
            }
            */
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
