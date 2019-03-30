// Librarys
//************************************************************************************
#include "MedtronicAPS.h"
//************************************************************************************
// Bluetooth device name
const String deviceName = "MedtronicESP"; // Device name
#define doDebug true
#ifdef doDebug
#define serialBaud 115200
#endif
//************************************************************************************
// RTC data variables (Persistent variables)
RTC_DATA_ATTR bool pumpOn = true;
RTC_DATA_ATTR float tempBasal; // Temp basal rate, in U/h
RTC_DATA_ATTR uint8_t tempDuration; // Duration of temp basal, in min.
RTC_DATA_ATTR bool tempActive = false;
RTC_DATA_ATTR uint64_t tempStart; // Temp start time
//************************************************************************************
// Library instance initialization
MedtronicAPS medtronic(doDebug);
//************************************************************************************
// Setup
void setup() {
    #ifdef doDebug
        Serial.begin(serialBaud);
    #endif
}
//************************************************************************************
// Main loop
void loop() {
    medtronic.maintainConnection();
}
//************************************************************************************