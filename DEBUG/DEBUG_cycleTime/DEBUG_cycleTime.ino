//Librarys
//hist: 20190208 lasse
//--------------------------------------------------------------------------------------------------------------
#include <math.h>
#include <BluetoothSerial.h>
//**************************************************************************************************************
//Bluetooth serial variables
//--------------------------------------------------------------------------------------------------------------
const String deviceName = "BluetoothPumpESP"; //Defines the name of the device
//const String deviceKey = "DSjk4398@jkf";
//***************************************************************************************************************
//Treatment variables
//--------------------------------------------------------------------------------------------------------------
#define M_TO_uS_FACTOR 60000000
#define uS_TO_S_FACTOR 1000000

RTC_DATA_ATTR float baseBasal;      //Pump base basal rate, in U/h
RTC_DATA_ATTR float tempBasal;      //Temp basal rate, in U/h
RTC_DATA_ATTR uint8_t tempDuration; //Duration of temp basal, in min.
RTC_DATA_ATTR uint8_t tempStart;    //Duration of temp basal, in min.
//bool pumpActive = true;
RTC_DATA_ATTR bool tempActive = false;

uint8_t wakeInterval = 1;

//const float minBolus = 0.1; //Minimum bolus delivery possible
//const float maxDeltaBasal = 5.0; //Maximum delta basal rate. --####Rettet####
//const uint8_t minTimeInterval = 4; //Minimum time interval bewteen bolus delivery --####Rettet####

//float deltaBasal; //Difference between baseBasal and tempBasal
//float bolusAmount; //Amount of U to deliver
//uint8_t bolusAmountScaler;
//uint8_t bolusCount; //Number of times to deliver minBolus
//uint8_t bolusTimeInterval; //Time between each minBolus, in minuts.
//uint8_t bolusDelivered; //Tracker for amount of times bolus has been delivered

//unsigned long firstTreatmentTime; //Time of first bolus delivery
//unsigned long prevTreatmentTime; //Previous treatment time, in milliseconds.
//****PKT 1512**********************************************************************************************************
const int B = 19;   // Could be different depending on the dev board. I used the DOIT ESP32 dev board.
const int S = 17;   // Could be different depending on the dev board. I used the DOIT ESP32 dev board.
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

#define handshakeInterval 1000 //Milliseconds between handshake attempt
bool handshakingCompleted = false;
uint64_t lastMessageTime;
#define resetTimeScaler 2
uint64_t wakeTime;

uint64_t currentMillis;

#define ESP_battery "e="
#define ESP_wake "w="
#define ESP_base "b="
#define ESP_temp "t="
#define ESP_sleep "s"

#define APS_ping "P"
#define APS_base "B="
#define APS_temp "T="
#define APS_wake "W="
#define APS_sleep "S"

#define comm_variable1 ":0="


unsigned long timer1;
unsigned long timer2;

//**************************************************************************************************************
//Library instance initialization
//--------------------------------------------------------------------------------------------------------------
BluetoothSerial SerialBT;
//**************************************************************************************************************
//Setup bluetooth serial
void setupBluetooth()
{
    SerialBT.begin(deviceName); //Starts bluetooth serial
    delay(200);                 // wait for voltage stabilize
#ifdef print_bluetooth
    Serial.print("BluetoothSerial started with device name: ");
    Serial.println(deviceName);
#endif
}
//**************************************************************************************************************
//Read from bleutooth serial and add to buffer
void readBluetooth(String dataString = "")
{
    boolean dataAvaliable = false;
    uint8_t btData;
    while (SerialBT.available())
    { //Run when data in buffer
        dataAvaliable = true;
        btData = SerialBT.read(); //Add data to variable
        if (btData == 13)
        {                                 //New line marks end of string
            processBluetooth(dataString); //Process it string
            return;                       //Bail out of while loop
        }
        dataString.concat(ASCIIintToChar(btData)); //Add data to string
    }
    if (dataAvaliable && btData != 13)
    {                              //String not fully recieved
        delay(10);                 //Wait a short time
        readBluetooth(dataString); //Return to bluetooth reader
    }
}
//**************************************************************************************************************
//Process string
void processBluetooth(String command)
{
#ifdef debug_serial
    Serial.print("Got BT string: ");
    Serial.println(command);
#endif
    if (command.indexOf(APS_ping) >= 0)
    {
        //
    }
    else if (command.indexOf(APS_base) >= 0)
    {
        baseBasalRate(command);
    }
    else if (command.indexOf(APS_temp) >= 0)
    {
        if (command.indexOf("null") >= 0)
        {
            cancelTempBasal();
        }
        else
        {
            newTempBasal(command);
        }
    }
    else if (command.indexOf(APS_wake) >= 0)
    {
        handshakingCompleted = true;
        updateWakeTimer(command);
    }
    else if (command.indexOf(APS_sleep) >= 0)
    {
        sleepNow();
    }
}
//**************************************************************************************************************
//Isolate value from string
float cutVariableFromString(String inputString, String leadingString, int sizeOfVariable, int type)
{
    //Example string: "getBasalRate rate: 0.9 duration: 30"
    //Isolate 0.9
    inputString.remove(0, inputString.indexOf(leadingString) + leadingString.length());
    //Example string: "0.9 duration: 30"
    if (inputString.length() != sizeOfVariable)
    {
        inputString.remove(sizeOfVariable, inputString.length());
    }
    if (type == 0)
    {
        return inputString.toInt();
    }
    else if (type == 1)
    {
        return inputString.toFloat();
    }
}
//**************************************************************************************************************
//Isolate base basal rate
void baseBasalRate(String command)
{
    if (command.length() > 2)
    {
        float newBaseBasal = cutVariableFromString(command, APS_base, 4, vFloat);
        if (newBaseBasal != baseBasal)
        {
#ifdef debug_serial
            Serial.print("New baseBasal rate set to ");
            Serial.print(newBaseBasal);
            Serial.print(" from ");
            Serial.print(baseBasal);
            Serial.println(".");
#endif
            baseBasal = newBaseBasal;
        }
    }
    command = ESP_base;
    command.concat(baseBasal);
    sendBluetooth(command);
}
//**************************************************************************************************************
//Isolate temp basal rate and duration
void newTempBasal(String command)
{
    if (command.length() > 2)
    {
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
    if (!tempActive)
    {
        command.concat("null");
    }
    else
    {
        command.concat(tempBasal);
        command.concat(comm_variable1);
        command.concat(tempDuration);
    }
    sendBluetooth(command);
}
//**************************************************************************************************************
//Set ACT
void setACT()
{
    digitalWrite(ACT, HIGH); // sets the ACT digital pin 18 on
    delay(3000);             // waits for 2 second
    digitalWrite(ACT, LOW);  // sets the ACT digital pin 18 OFF
    delay(3000);             // waits for 2 second
}
//**************************************************************************************************************
//Set B
void setB()
{
    digitalWrite(B, HIGH); // sets the B digital pin 19 on   ------- dettes skal gøres i X gange for af´t give rigt mængde ---------------------
    delay(3000);           // waits for 2 second            ------- dettes skal gøres i X gange for af´t give rigt mængde ---------------------PKT 1001  rettet til 2 istedet for 1
    digitalWrite(B, LOW);  // sets the B digital pin 19 OFF   ------- dettes skal gøres i X gange for af´t give rigt mængde ---------------------
    delay(3000);           // PKT 1001  rettet til 2 istedet for 1
}
//**************************************************************************************************************
//Set S
void setS()
{
    digitalWrite(S, HIGH); // sets the S digital pin 17 on
    delay(3000);           // waits for 2 second1  //// test med 5000 pkt 2112       PKT 1001  rettet til 2 istedet for 5
    digitalWrite(S, LOW);  // sets the S digital pin 17 on
    delay(3000);           //  PKT 1001  rettet til 2 istedet for 1     waits for 2 second
}
//**************************************************************************************************************
//Return to basal rate
void cancelTempBasal()
{
    tempActive = false;
    resetToDefault();
}
//**************************************************************************************************************
//Stop temp
void resetToDefault()
{

    /*
    setPumpStatus(pON);
    */
}
//**************************************************************************************************************
//Change status of pump - On and Off
/*
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
*/
//**************************************************************************************************************
//Turn off the pump
/*
void changePumpStatus() {
    setACT();
    for (uint8_t i = 0; i < 2; i++) {// Loop to do "something" n times
        setS();
    }
    setACT();
}
*/
//**************************************************************************************************************
//Handle bolus delivery
/*
void manageTreatment() {
    if ((millis() - prevTreatmentTime >= bolusTimeInterval * min_to_ms) && bolusDelivered < bolusCount) {
        deliverBolus();
    }
}
*/
//**************************************************************************************************************
//Handle continous pump action
/*
void managePump() {
    if (tempActive && (millis() - firstTreatmentTime >= tempDuration * min_to_ms)){
        tempActive = false;
        Serial.println("Temp over - Resetting to default");
        resetToDefault(); //Cancel temp and return pump to default state
    } else if (tempActive){
        manageTreatment();
    }
}
*/
//**************************************************************************************************************
void setupHardware()
{
    pinMode(18, OUTPUT);
    pinMode(B, OUTPUT);
    pinMode(ACT, OUTPUT);
    pinMode(S, OUTPUT);
}
//**************************************************************************************************************
//Transmit bluetooth message to host
void sendBluetooth(String message)
{
    for (uint8_t i = 0; i < message.length(); i++)
    {
        SerialBT.write(message[i]);
    }
    SerialBT.write(13);
}
//**************************************************************************************************************
//Update timer
void updateWakeTimer(String command)
{
    wakeInterval = cutVariableFromString(command, ESP_wake, 1, vInt);
    sendWake();
}

void sendWake()
{
    String command = ESP_wake;
    command.concat(wakeInterval);
    sendBluetooth(command);
}

void sleepNow()
{
#ifdef debug_serial
    Serial.println("Going to sleep!");
    Serial.print("Waking in: ");
    Serial.print(wakeInterval);
    Serial.println(" min");
#endif
    esp_sleep_enable_timer_wakeup(wakeInterval * M_TO_uS_FACTOR);
    esp_deep_sleep_start();
}

void handshake()
{
    if (!handshakingCompleted && currentMillis - lastMessageTime >= handshakeInterval)
    {
#ifdef debug_serial
        Serial.println("Handshaking!...");
#endif
        sendWake();
        lastMessageTime = millis();
    }
    if (currentMillis - wakeTime >= wakeInterval * resetTimeScaler)
    { //AndroidAPS didn't connect, reset
        handshakingCompleted = false;
    }
}

void handleTreatment()
{
    if ((currentMillis - tempStart) >= ((tempDuration * min_to_ms) + 30000)) {
        
    }
}

//**************************************************************************************************************
//Setup
void setup()
{
    Serial.begin(115200);
    setupBluetooth();
    setupHardware();
    delay(100);
    wakeTime = millis();
    timer1 = micros();
    currentMillis = millis();
    handshake();
    readBluetooth();
    handleTreatment();
    timer2 = micros();
    Serial.print("One cycle took: ");
    Serial.print(timer2 - timer1);
    Serial.println(" μs");
}
//**************************************************************************************************************
//Main loop
void loop()
{

}
//**************************************************************************************************************
//Converts a int value to a char
char ASCIIintToChar(uint8_t input)
{
    switch (input)
    { //Converts from int to char using ASCII
    case 10:
        return '\n';
    case 13:
        return '\r';
    case 32:
        return ' ';
    case 33:
        return '!';
    case 34:
        return '"';
    case 35:
        return '#';
    case 36:
        return '$';
    case 37:
        return '%';
    case 38:
        return '&';
    case 40:
        return '(';
    case 41:
        return ')';
    case 42:
        return '*';
    case 43:
        return '+';
    case 44:
        return ',';
    case 45:
        return '-';
    case 46:
        return '.';
    case 47:
        return '/';
    case 48:
        return '0';
    case 49:
        return '1';
    case 50:
        return '2';
    case 51:
        return '3';
    case 52:
        return '4';
    case 53:
        return '5';
    case 54:
        return '6';
    case 55:
        return '7';
    case 56:
        return '8';
    case 57:
        return '9';
    case 58:
        return ':';
    case 59:
        return ';';
    case 60:
        return '<';
    case 61:
        return '=';
    case 62:
        return '>';
    case 63:
        return '?';
    case 64:
        return '@';
    case 65:
        return 'A';
    case 66:
        return 'B';
    case 67:
        return 'C';
    case 68:
        return 'D';
    case 69:
        return 'E';
    case 70:
        return 'F';
    case 71:
        return 'G';
    case 72:
        return 'H';
    case 73:
        return 'I';
    case 74:
        return 'J';
    case 75:
        return 'K';
    case 76:
        return 'L';
    case 77:
        return 'M';
    case 78:
        return 'N';
    case 79:
        return 'O';
    case 80:
        return 'P';
    case 81:
        return 'Q';
    case 82:
        return 'R';
    case 83:
        return 'S';
    case 84:
        return 'T';
    case 85:
        return 'U';
    case 86:
        return 'V';
    case 87:
        return 'W';
    case 88:
        return 'X';
    case 89:
        return 'Y';
    case 90:
        return 'Z';
    case 91:
        return '[';
    case 93:
        return ']';
    case 94:
        return '^';
    case 95:
        return '_';
    case 96:
        return '`';
    case 97:
        return 'a';
    case 98:
        return 'b';
    case 99:
        return 'c';
    case 100:
        return 'd';
    case 101:
        return 'e';
    case 102:
        return 'f';
    case 103:
        return 'g';
    case 104:
        return 'h';
    case 105:
        return 'i';
    case 106:
        return 'j';
    case 107:
        return 'k';
    case 108:
        return 'l';
    case 109:
        return 'm';
    case 110:
        return 'n';
    case 111:
        return 'o';
    case 112:
        return 'p';
    case 113:
        return 'q';
    case 114:
        return 'r';
    case 115:
        return 's';
    case 116:
        return 't';
    case 117:
        return 'u';
    case 118:
        return 'v';
    case 119:
        return 'w';
    case 120:
        return 'x';
    case 121:
        return 'y';
    case 122:
        return 'z';
    case 123:
        return '{';
    case 124:
        return '|';
    case 125:
        return '}';
    case 126:
        return '~';
    default:
        return ' ';
    }
}