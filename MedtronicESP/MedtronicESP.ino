//************************************************************************************
// Librarys
#include <Arduino.h>
//#include "BluetoothInterface.h"
#include "PumpInterface.h"
#include "MakeLifeEasy.h"
//************************************************************************************
// BLE specific variables
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2904.h>

bool deviceConnected = false;

#define SERVICE_UUID           "27652cbb-76f0-45eb-bc37-826ca7315457" // UART service UUID
#define CHARACTERISTIC_UUID_RX "8983c612-5b25-43cd-85f0-391f8dd3cb67"
#define CHARACTERISTIC_UUID_TX "848909c1-a6f0-4fa4-ac2a-06b9a9d4eb60"

BLEServer *pServer;

BLECharacteristic *pInputCharacteristic;
BLECharacteristic *pOutputCharacteristic;
//************************************************************************************
#define doDebug true
#ifdef doDebug
    #define serialBaud 115200
#endif

#define M_TO_uS_FACTOR 60000000
#define uS_TO_S_FACTOR 1000000
#define min_to_ms 60000

const String ESP_battery = "e=";
const String ESP_temp = "t=";
const String ESP_bolus = "b=";
const String ESP_sleep = "s=";

const String APS_ping = "P";
const String APS_temp = "T=";
const String APS_bolus = "B=";
const String APS_sleep = "S=";
const String comm_variable1 = ":0=";

#define handshakeInterval 5000 //Milliseconds between handshake attempt
#define resetTimeScaler 2

#define cooldownTime 1

// Password for accepting commands
const String keyword = "123456"; // Device password

// Bluetooth device name
const String deviceName = "MedESP"; // Device name
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
//BluetoothInterface MedBlue;
//************************************************************************************
// Setup
void setup() {
    #ifdef doDebug
        Serial.begin(serialBaud);
    #endif
    // Start AndroidAPS communication
    pump.begin(15,14,32,27,33); // BOL, ACT, ESC, UP, DOWN
    //MedBlue.begin(deviceName, true);
    setupBluetooth(deviceName);
}
//************************************************************************************
// Main loop
void loop() {
    //processMessage(MedBlue.getMessage());
    #ifdef doDebug
        serialAction();
    #endif
}
//************************************************************************************
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
            gotPing();
        } else if (command.indexOf(APS_temp) >= 0) {
            gotTemp(command);
        } else if (command.indexOf(APS_bolus) >= 0) {
            gotBolus(command);
        } else if (command.indexOf(APS_sleep) >= 0) {
            gotSleep(command);
        }
    } else {
        #ifdef doDebug
            Serial.println(" - Wrong password");
        #endif
        gotWrongPassword();
    }
}
// Ping message, send battery status
void gotPing() {


    //MedBlue.sendMessage("e=100");
    sendMessage("e=100");
}
// Deliver bolus
void gotBolus(String command) {
    float bolus = getFloatfromStr(command, APS_temp, 4);
    pump.setBolus(bolus);
    command = ESP_bolus;
    if (!tempActive) { command.concat("null"); } 
    else {
        command.concat(bolus);
    }
    //MedBlue.sendMessage(command);
    sendMessage(command);
}
// Temp command
void gotTemp(String command) {
    if (command.indexOf("null") >= 0) {
        cancelTempBasal();
    } else {
        newTempBasal(command);
    }
}
// Isolate temp basal rate and duration
void newTempBasal(String command) {
    float basalRate = getFloatfromStr(command, APS_temp, 4);
    uint8_t duration = getIntfromStr(command, comm_variable1, 3);
    #ifdef doDebug
        newTempDebug(basalRate, duration);
    #endif
    pump.setTemp(basalRate, duration);
    command = ESP_temp;
    if (!tempActive) { command.concat("null"); } 
    else {
        command.concat(tempBasal);
        command.concat(comm_variable1);
        command.concat(tempDuration);
    }
    //MedBlue.sendMessage(command);
    sendMessage(command);
}
// Return to basal rate
void cancelTempBasal() {
    pump.cancelTemp();
}
// Send sleep message and go to sleep
void gotSleep(String command) {
    uint8_t wakeInterval = getIntfromStr(command, APS_sleep, 1);
    command = ESP_sleep;
    command.concat(wakeInterval);
    //MedBlue.sendMessage(ESP_sleep);
    sendMessage(command);
    delay(100); // Wait a short time to make sure message was sendt before shutting down
    #ifdef doDebug
        sleepNowDebug(wakeInterval); 
    #endif
    sleepESP(wakeInterval);
}
// Put ESP to sleep
void sleepESP(uint8_t sleepTime) {
    stopBluetooth();
    esp_sleep_enable_timer_wakeup(sleepTime * M_TO_uS_FACTOR);
    esp_deep_sleep_start();
}

void gotWrongPassword() {
    sleepESP(cooldownTime);
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

//************************************************************************************
// BLE specific functions
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
        deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
        deviceConnected = false;
    }
};

class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
        std::string rxValue = pCharacteristic->getValue();
        if (rxValue.length() > 0) {
            Serial.print("Received Value: ");
            for (int i = 0; i < rxValue.length(); i++) {
                Serial.print(rxValue[i]);
            }
            Serial.println();
            processMessage(rxValue.c_str());
        }
    }
};

void setupBluetooth(String mDeviceName) {
    // Create the BLE Device
    uint8_t size = mDeviceName.length()+1;
    char charName[size];
    mDeviceName.toCharArray(charName, size);
    charName[size] = '\0';
    BLEDevice::init(charName); // Give it a name
    // Create the BLE Server
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());
    // Create the BLE Service
    BLEService *pService = pServer->createService(SERVICE_UUID);
    // Create input BLE Characteristic
    pInputCharacteristic = pService->
        createCharacteristic(CHARACTERISTIC_UUID_RX, BLECharacteristic::PROPERTY_WRITE);
    // Create output BLE Characteristic
    pOutputCharacteristic = pService->
        createCharacteristic(CHARACTERISTIC_UUID_TX, BLECharacteristic::PROPERTY_NOTIFY 
                                                   | BLECharacteristic::PROPERTY_READ);
    // Add descriptors
    pInputCharacteristic->setCallbacks(new MyCallbacks());
    pOutputCharacteristic->addDescriptor(new BLE2904());
    // Add service UUID to advertising 
    pServer->getAdvertising()->addServiceUUID(SERVICE_UUID);
    // Start the service
    pService->start();
    // Start advertising
    pServer->getAdvertising()->start();
    Serial.println("BLE started");
}

void stopBluetooth() {
    Serial.println("Stopping BLE");
    pServer->getAdvertising()->stop();
}

bool sendMessage(String message) {
    if (deviceConnected) {
        uint8_t size = message.length()+1;
        char charMessage[size];
        message.toCharArray(charMessage, size);
        charMessage[size] = '\0';
        pOutputCharacteristic->setValue(charMessage);
        pOutputCharacteristic->notify(); // Send the value to the app!
        Serial.print("*** Sent string: ");
        Serial.println(message);
        return true;
    }
    return false;
}