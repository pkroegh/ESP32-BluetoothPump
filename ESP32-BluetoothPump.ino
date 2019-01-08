//Librarys
//--------------------------------------------------------------------------------------------------------------
#include <math.h>
#include <BluetoothSerial.h>
//**************************************************************************************************************
//Bluetooth serial variables
//--------------------------------------------------------------------------------------------------------------
String deviceName = "BluetoothPumpESP"; //Defines the name of the device
char arrayOK[5] = {"OK\r\n"};
//***************************************************************************************************************
//Treatment variables
//--------------------------------------------------------------------------------------------------------------
float baseBasal; //Pump base basal rate, in U/h
float tempBasal; //Temp basal rate, in U/h
uint8_t tempDuration; //Duration of temp basal, in min.
bool pumpActive = true;
bool tempActive = false;

const float minBolus = 0.1; //Minimum bolus delivery possible
const float maxDeltaBasal = 3.0; //Maximum delta basal rate.
const uint8_t minTimeInterval = 5; //Minimum time interval bewteen bolus delivery

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
    #ifndef ignore_confirm
        confirmRecieved();
    #endif
    #ifdef print_bluetooth
        Serial.print("Got string: ");
        Serial.println(command);
    #endif
    if (command.indexOf("getBaseBasalRate") >= 0) {
        baseBasalRate(command);
    } else if (command.indexOf("setTempBasalAbsolute") >= 0) {
        newTempBasal(command);
    } else if (command.indexOf("cancelTempBasal") >= 0) {
        cancelTempBasal();
    }
}
//**************************************************************************************************************
//Confirm message
#ifndef ignore_confirm
    void confirmRecieved() {
        SerialBT.write(arrayOK[0]);
        SerialBT.write(arrayOK[1]);
        SerialBT.write(arrayOK[2]);
        SerialBT.write(arrayOK[3]);
    }
#endif
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
    Serial.print("Basal rate isolated to: ");
    Serial.println(newBaseBasal);
    if (newBaseBasal != baseBasal) {
        baseBasal = newBaseBasal;
        Serial.println("New baseBasal rate");
    }
}
//**************************************************************************************************************
//Isolate temp basal rate and duration
void newTempBasal(String command) {
    tempBasal = cutVariableFromString(command, "Basal: ", 4, vFloat);
    tempDuration = cutVariableFromString(command, "tion: ", 3, vInt);
    #ifdef debug_serial
        Serial.println("Temporary basal rate update recieved;");
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
        
        bolusAmountScaler = (float)bolusCount / (60 / (float)minTimeInterval) + 0.5;
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
    Serial.print("Delivering bolus: ");
    Serial.println(bolusAmount);
    setACT();
    for (uint8_t i = 0; i < bolusAmountScaler; i++) {
        setB();
    }
    setACT();
    bolusDelivered++;
    prevTreatmentTime = millis();
}
//**************************************************************************************************************
//Set ACT
void setACT(){
    digitalWrite(ACT, HIGH);      // sets the ACT digital pin 18 on
    delay(2000);                  // waits for 2 second
    digitalWrite(ACT, LOW);       // sets the ACT digital pin 18 OFF
    delay(2000);                  // waits for 2 second
}
//**************************************************************************************************************
//Set B
void setB(){
    digitalWrite(B, HIGH);       // sets the B digital pin 19 on   ------- dettes skal gøres i X gange for af´t give rigt mængde ---------------------
    delay(1000);                  // waits for 2 second            ------- dettes skal gøres i X gange for af´t give rigt mængde ---------------------
    digitalWrite(B, LOW);       // sets the B digital pin 19 OFF   ------- dettes skal gøres i X gange for af´t give rigt mængde ---------------------
    delay(1000);    
}
//**************************************************************************************************************
//Set S
void setS(){
    digitalWrite(S, HIGH);       // sets the S digital pin 17 on
    delay(5000);                  // waits for 2 second1  //// test med 5000 pkt 2112
    digitalWrite(S, LOW);       // sets the S digital pin 17 on
    delay(1000);                  // waits for 2 second
}
//**************************************************************************************************************
//Return to basal rate
void cancelTempBasal() {
    resetToDefault();
}
//**************************************************************************************************************
//Stop temp and make sure pump is on
void resetToDefault() {
    setPumpStatus(pON);
    tempActive = false;
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
    if ((millis() - prevTreatmentTime >= bolusTimeInterval * min_to_ms) && bolusDelivered <= bolusCount) {
        deliverBolus();
    }
}
//**************************************************************************************************************
//Handle continous pump action
void managePump() {
    if (tempActive) {
        manageTreatment();
    } else if (millis() - firstTreatmentTime >= tempDuration * min_to_ms){
        Serial.println("Temp over - Resetting to default");
        resetToDefault(); //Cancel temp and return pump to default state
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
    readBluetooth();
    managePump();
}
//**************************************************************************************************************
//Converts a int value to a char
char ASCIIintToChar(uint8_t input) {
    switch (input) { //Converts from int to char using ASCII
        case 10: return '\n';
        case 13: return '\r';
        case 32: return ' ';
        case 33: return '!';
        case 34: return '"';
        case 35: return '#';
        case 36: return '$';
        case 37: return '%';
        case 38: return '&';
        case 40: return '(';
        case 41: return ')';
        case 42: return '*';
        case 43: return '+';
        case 44: return ',';
        case 45: return '-';
        case 46: return '.';
        case 47: return '/';
        case 48: return '0';
        case 49: return '1';
        case 50: return '2';
        case 51: return '3';
        case 52: return '4';
        case 53: return '5';
        case 54: return '6';
        case 55: return '7';
        case 56: return '8';
        case 57: return '9';
        case 58: return ':';
        case 59: return ';';
        case 60: return '<';
        case 61: return '=';
        case 62: return '>';
        case 63: return '?';
        case 64: return '@';
        case 65: return 'A';
        case 66: return 'B';
        case 67: return 'C';
        case 68: return 'D';
        case 69: return 'E';
        case 70: return 'F';
        case 71: return 'G';
        case 72: return 'H';
        case 73: return 'I';
        case 74: return 'J';
        case 75: return 'K';
        case 76: return 'L';
        case 77: return 'M';
        case 78: return 'N';
        case 79: return 'O';
        case 80: return 'P';
        case 81: return 'Q';
        case 82: return 'R';
        case 83: return 'S';
        case 84: return 'T';
        case 85: return 'U';
        case 86: return 'V';
        case 87: return 'W';
        case 88: return 'X';
        case 89: return 'Y';
        case 90: return 'Z';
        case 91: return '[';
        case 93: return ']';
        case 94: return '^';
        case 95: return '_';
        case 96: return '`';
        case 97: return 'a';
        case 98: return 'b';
        case 99: return 'c';
        case 100: return 'd';
        case 101: return 'e';
        case 102: return 'f';
        case 103: return 'g';
        case 104: return 'h';
        case 105: return 'i';
        case 106: return 'j';
        case 107: return 'k';
        case 108: return 'l';
        case 109: return 'm';
        case 110: return 'n';
        case 111: return 'o';
        case 112: return 'p';
        case 113: return 'q';
        case 114: return 'r';
        case 115: return 's';
        case 116: return 't';
        case 117: return 'u';
        case 118: return 'v';
        case 119: return 'w';
        case 120: return 'x';
        case 121: return 'y';
        case 122: return 'z';
        case 123: return '{';
        case 124: return '|';
        case 125: return '}';
        case 126: return '~';
        default: return ' ';
    }
}
//**************************************************************************************************************
//Converts a char to an int
uint8_t ASCIIcharToInt(char input) {
    switch (input) { //Converts from char to int using ASCII
        case '\n': return 10;
        case '\r': return 13;
        case ' ': return 32;
        case '!': return 33;
        case '"': return 34;
        case '#': return 35;
        case '$': return 36;
        case '%': return 37;
        case '&': return 38;
        case '(': return 40;
        case ')': return 41;
        case '*': return 42;
        case '+': return 43;
        case ',': return 44;
        case '-': return 45;
        case '.': return 46;
        case '/': return 47;
        case '0': return 48;
        case '1': return 49;
        case '2': return 50;
        case '3': return 51;
        case '4': return 52;
        case '5': return 53;
        case '6': return 54;
        case '7': return 55;
        case '8': return 56;
        case '9': return 57;
        case ':': return 58;
        case ';': return 59;
        case '<': return 60;
        case '=': return 61;
        case '>': return 62;
        case '?': return 63;
        case '@': return 64;
        case 'A': return 65;
        case 'B': return 66;
        case 'C': return 67;
        case 'D': return 68;
        case 'E': return 69;
        case 'F': return 70;
        case 'G': return 71;
        case 'H': return 72;
        case 'I': return 73;
        case 'J': return 74;
        case 'K': return 75;
        case 'L': return 76;
        case 'M': return 77;
        case 'N': return 78;
        case 'O': return 79;
        case 'P': return 80;
        case 'Q': return 81;
        case 'R': return 82;
        case 'S': return 83;
        case 'T': return 84;
        case 'U': return 85;
        case 'V': return 86;
        case 'W': return 87;
        case 'X': return 88;
        case 'Y': return 89;
        case 'Z': return 90;
        case '[': return 91;
        case ']': return 93;
        case '^': return 94;
        case '_': return 95;
        case '`': return 96;
        case 'a': return 97;
        case 'b': return 98;
        case 'c': return 99;
        case 'd': return 100;
        case 'e': return 101;
        case 'f': return 102;
        case 'g': return 103;
        case 'h': return 104;
        case 'i': return 105;
        case 'j': return 106;
        case 'k': return 107;
        case 'l': return 108;
        case 'm': return 109;
        case 'n': return 110;
        case 'o': return 111;
        case 'p': return 112;
        case 'q': return 113;
        case 'r': return 114;
        case 's': return 115;
        case 't': return 116;
        case 'u': return 117;
        case 'v': return 118;
        case 'w': return 119;
        case 'x': return 120;
        case 'y': return 121;
        case 'z': return 122;
        case '{': return 123;
        case '|': return 124;
        case '}': return 125;
        case '~': return 126;
        default: return ' ';
    }
}
