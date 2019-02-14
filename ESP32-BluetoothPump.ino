//Librarys
//hist: 20190208 lasse
//--------------------------------------------------------------------------------------------------------------
#include <math.h>
#include <BluetoothSerial.h>
//**************************************************************************************************************
//Bluetooth serial variables
//--------------------------------------------------------------------------------------------------------------
const String deviceName = "BluetoothPumpESP"; //Defines the name of the device
const String deviceKey = "DSjk4398@jkfd%";
//***************************************************************************************************************
//Treatment variables
//--------------------------------------------------------------------------------------------------------------
float baseBasal; //Pump base basal rate, in U/h
float tempBasal; //Temp basal rate, in U/h
uint8_t tempDuration; //Duration of temp basal, in min.
bool pumpActive = true;
bool tempActive = false;

const float minBolus = 0.1; //Minimum bolus delivery possible
const float maxDeltaBasal = 5.0; //Maximum delta basal rate. --####Rettet####
const uint8_t minTimeInterval = 4; //Minimum time interval bewteen bolus delivery --####Rettet####

float deltaBasal; //Difference between baseBasal and tempBasal
float bolusAmount; //Amount of U to deliver
uint8_t bolusAmountScaler;
uint8_t bolusCount; //Number of times to deliver minBolus
uint8_t bolusTimeInterval; //Time between each minBolus, in minuts.
uint8_t bolusDelivered; //Tracker for amount of times bolus has been delivered

unsigned long firstTreatmentTime; //Time of first bolus delivery
unsigned long prevTreatmentTime; //Previous treatment time, in milliseconds.
//****PKT 1512**********************************************************************************************************
const int B = 19; // Could be different depending on the dev board. I used the DOIT ESP32 dev board.
const int S = 17; // Could be different depending on the dev board. I used the DOIT ESP32 dev board.
const int ACT = 16; // Could be different depending on the dev board. I used the DOIT ESP32 dev board.
//****PKT 1512**********************************************************************************************************
#define vInt 0
#define vFloat 1
#define pOFF 0
#define pON 1
#define min_to_ms 60000
//**************************************************************************************************************
//DEBUG variables
//--------------------------------------------------------------------------------------------------------------
#define debug_serial
#define ignore_confirm
#ifdef debug_serial
#define print_setup
#define print_bluetooth
#endif
//**************************************************************************************************************
//Library instance initialization
//--------------------------------------------------------------------------------------------------------------
BluetoothSerial SerialBT;
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
//Read from bleutooth serial and add to buffer
void readBluetooth(String dataString = "") {
    boolean dataAvaliable = false;
    uint8_t btData;
    while (SerialBT.available()) { //Run when data in buffer
        dataAvaliable = true;
        btData = SerialBT.read(); //Add data to variable
        if (btData == 10) { //New line marks end of string
        processBluetooth(dataString); //Process it string
        return; //Bail out of while loop
        }
        dataString.concat(ASCIIintToChar(btData)); //Add data to string
    }
    if (dataAvaliable && btData != 10) { //String not fully recieved
        delay(10); //Wait a short time
        readBluetooth(dataString); //Return to bluetooth reader
    }
}
//**************************************************************************************************************
//Process string
void processBluetooth(String command) {
    if (command.indexOf(deviceKey) >= 0) {
        if (command.indexOf("ping") >= 0) {
            //
        } else if (command.indexOf("base") >= 0) {
            baseBasalRate(command);
        } else if (command.indexOf("temp") >= 0) {
            if (command.indexOf("null") >= 0) {
                cancelTempBasal();
            } else {
                newTempBasal(command);
            }
        } else if (command.indexOf("wake") >= 0) {
            updateWakeTimer(command);
        } else if (command.indexOf("slub") >= 0) {
            //
        }
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
//Isolate base basal rate
void baseBasalRate(String command) {
    float newBaseBasal = cutVariableFromString(command, "rate: ", 4, vFloat);
    if (newBaseBasal != baseBasal) {
        Serial.print("New baseBasal rate set to ");
        Serial.print(newBaseBasal);
        Serial.print(" from ");
        Serial.print(baseBasal);
        Serial.println(".");
        baseBasal = newBaseBasal;
        sendBluetooth("base=");
        sendBluetooth(baseBasal);
        okayBluetooth();
    }
}
//**************************************************************************************************************
//Isolate temp basal rate and duration
void newTempBasal(String command) {
    tempBasal = cutVariableFromString(command, "Basal: ", 4, vFloat);
    tempDuration = cutVariableFromString(command, "tion: ", 3, vInt);
    #ifdef debug_serial
        Serial.println("Temporary basal rate update recieved!");
        Serial.print("TempBasal to be set: ");
        Serial.print(tempBasal);
        Serial.print("U/h for tempDuration: ");
        Serial.print(tempDuration);
        Serial.println(" min");
    #endif
    if (tempBasal > baseBasal) {
        setPumpStatus(pON);
        calculateTempBasal();
        deliverBolus();
    } else if (tempBasal < baseBasal) {
        setPumpStatus(pOFF);
    }
}
//**************************************************************************************************************
//Calculate bolusAmount, bolusCount and bolusTimeInterval
void calculateTempBasal(){
    deltaBasal = tempBasal - baseBasal; //Calcualte difference in basal rate
    if (deltaBasal > maxDeltaBasal) { //Make sure that deltaBasal does not become larger then maxDeltaBasal
        deltaBasal = maxDeltaBasal;
    }
    bolusCount = deltaBasal / minBolus;
    bolusTimeInterval = 60 / bolusCount + 0.5;
    if(bolusTimeInterval < minTimeInterval){
        Serial.println("bolusTimeInterval < minTimeInterval! Recalculating!");
        Serial.println("Using larger minBolus");
        bolusAmountScaler = ((float)bolusCount / (60 / (float)minTimeInterval)) + 0.5;
        bolusAmount = minBolus * bolusAmountScaler;
        bolusCount = deltaBasal / bolusAmount;
        bolusTimeInterval = 60 / bolusCount;
    } else {
        bolusAmount = minBolus;
    }
    debug_printCalculation();
    bolusDelivered = 0;
    tempActive = true;
    firstTreatmentTime = millis();
}
//**************************************************************************************************************
//Print calculation from calculateTempBasal
void debug_printCalculation(){
    Serial.print("deltaBasal: ");
    Serial.println(deltaBasal);
    Serial.print("bolusAmount: ");
    Serial.println(bolusAmount);
    Serial.print("bolusCount: ");
    Serial.println(bolusCount);
    Serial.print("bolusTimeInterval: ");
    Serial.println(bolusTimeInterval);
}
//**************************************************************************************************************
//Deliver bolus treatment
void deliverBolus(){
    Serial.print("Delivering ");
    Serial.print(bolusAmount);
    Serial.print(" U of bolus. Count ");
    Serial.print(bolusDelivered+1);
    Serial.print(" of ");
    Serial.print(bolusCount);
    Serial.println(".");
    setACT();
    for (uint8_t i = 0; i < (bolusAmountScaler); i++) {         //PKT 1001 tilføjet+1 da den ikke vil tænde diode hvis det er 0,1 men  godt hvis det er 0,2 - fjernet igen
        setB();
    }
    setACT();
    setACT();       //PKT 1701 tilføjet ser ud som om act skal sætte 2 gang på fjernbetjening
    bolusDelivered++;
    prevTreatmentTime = millis();
}
//**************************************************************************************************************
//Set ACT
void setACT(){
    digitalWrite(ACT, HIGH);      // sets the ACT digital pin 18 on
    delay(3000);                  // waits for 2 second
    digitalWrite(ACT, LOW);       // sets the ACT digital pin 18 OFF
    delay(3000);                  // waits for 2 second
}
//**************************************************************************************************************
//Set B
void setB(){
    digitalWrite(B, HIGH);       // sets the B digital pin 19 on   ------- dettes skal gøres i X gange for af´t give rigt mængde ---------------------
    delay(3000);                  // waits for 2 second            ------- dettes skal gøres i X gange for af´t give rigt mængde ---------------------PKT 1001  rettet til 2 istedet for 1
    digitalWrite(B, LOW);       // sets the B digital pin 19 OFF   ------- dettes skal gøres i X gange for af´t give rigt mængde ---------------------
    delay(3000);                // PKT 1001  rettet til 2 istedet for 1
}
//**************************************************************************************************************
//Set S
void setS(){
    digitalWrite(S, HIGH);       // sets the S digital pin 17 on
    delay(3000);                  // waits for 2 second1  //// test med 5000 pkt 2112       PKT 1001  rettet til 2 istedet for 5
    digitalWrite(S, LOW);       // sets the S digital pin 17 on
    delay(3000);                  //  PKT 1001  rettet til 2 istedet for 1     waits for 2 second
}
//**************************************************************************************************************
//Return to basal rate
void cancelTempBasal() {
    tempActive = false;
    resetToDefault();
}
//**************************************************************************************************************
//Stop temp and make sure pump is on
void resetToDefault() {
    setPumpStatus(pON);
}
//**************************************************************************************************************
//Change status of pump - On and Off
void setPumpStatus(uint8_t state) { //If state is ON -> Turn on the pump. If state is OFF -> Turn off the pump
    if (!pumpActive && state == pON) { //If we want to start the pump, and the pump is off, start it with base basal rate
        Serial.println("Starting pump");
        changePumpStatus();
        pumpActive = true;
    } else if (pumpActive && state == pOFF) { //If we want to turn off the pump, and the pump is on, turn off the pump.
        Serial.println("Stopping pump");
        changePumpStatus();
        pumpActive = false;
    }
}
//**************************************************************************************************************
//Turn off the pump
void changePumpStatus() {
    setACT();
    for (uint8_t i = 0; i < 2; i++) {// Loop to do "something" n times
        setS();
    }
    setACT();
}
//**************************************************************************************************************
//Handle bolus delivery
void manageTreatment() {
    if ((millis() - prevTreatmentTime >= bolusTimeInterval * min_to_ms) && bolusDelivered < bolusCount) {
        deliverBolus();
    }
}
//**************************************************************************************************************
//Handle continous pump action
void managePump() {
    if (tempActive && (millis() - firstTreatmentTime >= tempDuration * min_to_ms)){
        tempActive = false;
        Serial.println("Temp over - Resetting to default");
        resetToDefault(); //Cancel temp and return pump to default state
    } else if (tempActive){
        manageTreatment();
    }
}
//**************************************************************************************************************
void setupHardware(){
    pinMode(18, OUTPUT);
    pinMode(B, OUTPUT);
    pinMode(ACT, OUTPUT);
    pinMode(S, OUTPUT);
}
//**************************************************************************************************************
//Transmit bluetooth message to host
void sendBluetooth(String message, bool sendSpace = false) {
    for (uint8_t i = 0; i < message.length(); i++){
        SerialBT.write(message[i]);
    }
    if (sendSpace) {
        SerialBT.write(32);
    }
}
//**************************************************************************************************************
void endMessage(){
    SerialBT.write(13);
}
//**************************************************************************************************************
void okayBluetooth() {
    sendBluetooth("okay");
    endMessage();
}
/*
//**************************************************************************************************************
//Send alive and current config
void pumpStatus(){
    sendBluetooth("Stauts: Alive");
    sendBluetooth()
}

void beginMessage(){
    sendBluetooth("Stauts: Alive");

}



void







*/

//**************************************************************************************************************
//Update timer
void updateWakeTimer(String command){
    uint8_t = wakeTimeMin = cutVariableFromString(command, "wake=", 1, vInt);
    

}


//**************************************************************************************************************
//Setup
void setup() {
    #ifdef debug_serial
        Serial.begin(115200);
    #endif
    setupBluetooth();
    setupHardware();
}
//**************************************************************************************************************
//Main loop
void loop() {
    //readBluetooth();
    //managePump();
    sendBluetooth("Test with a number. The number in string 143. The number in dec: ");
    SerialBT.write(49);
    SerialBT.write(52);
    SerialBT.write(51);
    SerialBT.write(13);
    delay(8000);
}
//**************************************************************************************************************
