//Librarys
//--------------------------------------------------------------------------------------------------------------
#include <math.h>
#include <BluetoothSerial.h>
#include "definitions.h"
#include "esp_pinout.h"
#include "debug.h"
#include "ASCII.h"
//**************************************************************************************************************
//Bluetooth device name
//--------------------------------------------------------------------------------------------------------------
const String deviceName = "MedtronicESP"; //Defines the name of the device
//**************************************************************************************************************
//RTC data variables (Persistent variables)
//--------------------------------------------------------------------------------------------------------------
//RTC_DATA_ATTR float baseBasal; //Pump base basal rate, in U/h
RTC_DATA_ATTR float tempBasal; //Temp basal rate, in U/h
RTC_DATA_ATTR uint8_t tempDuration; //Duration of temp basal, in min.
RTC_DATA_ATTR bool tempActive = false;
//**************************************************************************************************************
//Temp variables
//--------------------------------------------------------------------------------------------------------------
uint8_t wakeInterval = 1;
#define handshakeInterval 3000 //Milliseconds between handshake attempt3
#define resetTimeScaler 2
bool handshakingCompleted = false;
uint64_t lastMessageTime;
uint64_t wakeTime;
uint64_t currentMillis;
//**************************************************************************************************************
//Library instance initialization
//--------------------------------------------------------------------------------------------------------------
BluetoothSerial SerialBT;
//**************************************************************************************************************
//Read from bleutooth serial and add to buffer
void readBluetooth(String dataString = "") {
    boolean dataAvaliable = false;
    uint8_t btData;
    while (SerialBT.available()) { //Run when data in buffer
        dataAvaliable = true;
        btData = SerialBT.read(); //Add data to variable
        if (btData == 13) { //New line marks end of string
            processBluetooth(dataString); //Process it string
            return; //Bail out of while loop
        }
        dataString.concat(ASCIIintToChar(btData)); //Add data to string
    }
    if (dataAvaliable && btData != 13) { //String not fully recieved
        delay(10); //Wait a short time
        readBluetooth(dataString); //Return to bluetooth reader
    }
}
//**************************************************************************************************************
//Process string
void processBluetooth(String command) {
    #ifdef debug_serial
        Serial.print("Got BT string: ");
        Serial.println(command);
    #endif
    if (command.indexOf(APS_ping) >= 0) {
        handshakingCompleted = true;
        sendBluetooth("e=100");
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
//**************************************************************************************************************
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
//**************************************************************************************************************
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
    }
    command = ESP_temp;
    if (!tempActive) {
        command.concat("null");
    } else {
        command.concat(tempBasal);
        command.concat(comm_variable1);
        command.concat(tempDuration);
    }
    sendBluetooth(command);
}
//**************************************************************************************************************
//Return to basal rate
void cancelTempBasal() {
    tempActive = false;
    resetToDefault();
}
//**************************************************************************************************************
//Stop temp
void resetToDefault() {

    /*
    setPumpStatus(pON);
    */
}
//**************************************************************************************************************
//Transmit bluetooth message to host
void sendBluetooth(String message) {
    for (uint8_t i = 0; i < message.length(); i++){
        #ifdef debug_serial
            char buff = message[i];
            Serial.print(buff);
            SerialBT.write(buff);
        #else
            SerialBT.write(message[i]);
        #endif
    }
    SerialBT.write(13);
}
//**************************************************************************************************************
//Update timer
void updateWakeTimer(String command) {
    wakeInterval = cutVariableFromString(command, APS_wake, 1, vInt);
    sendWake();
}
//**************************************************************************************************************
void sendWake() {
    String command = ESP_wake;
    command.concat(wakeInterval);
    sendBluetooth(command);
}
//**************************************************************************************************************
void sleepNow() {
    #ifdef debug_serial
        Serial.println("Going to sleep!");
        Serial.print("Waking in: ");
        Serial.print(wakeInterval);
        Serial.println(" min");
    #endif
    sendBluetooth(ESP_sleep);
    esp_sleep_enable_timer_wakeup(wakeInterval * M_TO_uS_FACTOR);
    esp_deep_sleep_start();
}
//**************************************************************************************************************
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
//**************************************************************************************************************
//Setup bluetooth serial
void setupBluetooth() {
    SerialBT.begin(deviceName); //Starts bluetooth serial
    delay(200); // wait for voltage stabilize
    #ifdef print_bluetooth
        Serial.print("BluetoothSerial started with device name: ");
        Serial.println(deviceName);
    #endif
}
//**************************************************************************************************************
//Setup
void setup() {
    #ifdef debug_serial
        Serial.begin(115200);
    #endif
    setupBluetooth();
    setupHardware();
    wakeTime = millis();
}
//**************************************************************************************************************
//Main loop
void loop() {
    currentMillis = millis();
    handshake();
    readBluetooth();
}
//**************************************************************************************************************