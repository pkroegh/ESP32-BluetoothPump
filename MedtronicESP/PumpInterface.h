// Pump <-> ESP interface

// ensure this library description is only included once
#ifndef PumpInterface_h
#define PumpInterface_h

#include <Arduino.h>

#ifndef M_TO_uS_FACTOR
    #define M_TO_uS_FACTOR 60000000
#endif
#ifndef uS_TO_S_FACTOR
    #define uS_TO_S_FACTOR 1000000
#endif
#ifndef min_to_ms
    #define min_to_ms 60000
#endif

#define defaultBOL 1
#define defaultACT 2
#define defaultESC 3
#define defaultUP 4
#define defualtDOWN 5

#define press_time 100 // Time from press to release
#define press_delay 300 // Time between presses

#define durationStepInterval 30
#define tempBasalInterval 0.025
#define bolusStepInterval 0.05

class PumpInterface {
    public:
        PumpInterface(bool *pumpOn, uint8_t *tempDuration,
                      bool *tempActive, uint64_t *tempStart);

        void begin();
        void begin(uint8_t BOLpin, uint8_t ACTpin, uint8_t ESCpin, 
                   uint8_t UPpin, uint8_t DOWNpin);
        void setTemp(float basalRate, uint8_t duration);
        void cancelTemp();
        void setBolus(float amount);
        bool stopPump();
        bool startPump();

        bool debug_hardware(char action);

    private:
        uint8_t BOL;
        uint8_t ACT;
        uint8_t ESC;
        uint8_t UP;
        uint8_t DOWN;

        float _tempBasal;
        bool *_pumpOn;
        uint8_t *_tempDuration;
        bool *_tempActive;
        uint64_t *_tempStart;

        void pressBOL();
        void pressACT();
        void pressESC();
        void pressUP();
        void pressDOWN();
        void escToMain();
        bool hasTempExpired();
};
#endif