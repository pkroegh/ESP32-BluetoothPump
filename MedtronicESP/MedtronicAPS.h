// Pump <-> AndroidAPS interface
// AndroidAPS command processing library

// TODO: Rename to MedtronicAPS

// ensure this library description is only included once
#ifndef MedtronicAPS_h
#define MedtronicAPS_h

#include <Arduino.h>
#include "BluetoothInterface.h"
#include "PumpInterface.h"

#ifndef M_TO_uS_FACTOR
    #define M_TO_uS_FACTOR 60000000
#endif
#ifndef uS_TO_S_FACTOR
    #define uS_TO_S_FACTOR 1000000
#endif
#ifndef min_to_ms
    #define min_to_ms 60000
#endif

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

#define handshakeInterval 3000 //Milliseconds between handshake attempt
#define resetTimeScaler 2

extern const String deviceName;
extern bool tempActive;
extern float tempBasal;
extern uint8_t tempDuration;

class MedtronicAPS {
    public:
        MedtronicAPS(void);
        MedtronicAPS(bool debug);

        void maintainConnection();
    private:
        PumpInterface pump;
        BluetoothInterface MedBlue;

        bool _debug = false;
        uint8_t wakeInterval = 1;
        bool handshakingCompleted = false;
        uint64_t lastMessageTime;
        uint64_t wakeTime;
        uint64_t currentMillis;
    
        void handshake();
        void processMessage(String command);
        void gotPing();
        void updateTemp(String command);
        void newTempBasal(String command);
        void cancelTempBasal();
        void updateWakeTimer(String command);
        void sendWake();
        void sleepNow();

        float cutVariableFromString(String inputString, String leadingString, 
                                    int sizeOfVariable, int type);

        void newTempDebug(float basalRate, uint8_t duration);
        void sleepNowDebug(uint8_t duration);
};
#endif