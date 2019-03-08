//Pinout and connections
//**************************************************************************************************************
//Pinout
//--------------------------------------------------------------------------------------------------------------
const int B = 19; // Could be different depending on the dev board. I used the DOIT ESP32 dev board.
const int S = 17; // Could be different depending on the dev board. I used the DOIT ESP32 dev board.
const int ACT = 16; // Could be different depending on the dev board. I used the DOIT ESP32 dev board.
//**************************************************************************************************************
//Press constants
//--------------------------------------------------------------------------------------------------------------
const int pressTime = 3000; //Time between press and release
const int pressDelay = 3000; //Time between release and next press
//**************************************************************************************************************
//Action functions
//--------------------------------------------------------------------------------------------------------------
//Decalre pinMode
void setupHardware(){
    pinMode(18, OUTPUT);
    pinMode(B, OUTPUT);
    pinMode(ACT, OUTPUT);
    pinMode(S, OUTPUT);


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