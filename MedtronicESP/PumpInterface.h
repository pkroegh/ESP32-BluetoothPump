// Pump <-> ESP interface

// ensure this library description is only included once
#ifndef PumpInterface_h
#define PumpInterface_h

#include <Arduino.h>

// Pinout from ESP to pump
#define BACK 1
#define ACT 2
#define ESC 3
#define UP 4
#define DOWN 5

#define press_time 100 // Time from press to release
#define press_delay 100 // Time between presses

#define durationStepInterval 30
#define tempBasalInterval 0.025
#define bolusStepInterval 0.05

class PumpInterface {
    public:
        PumpInterface();

        void setTemp(float basalRate, uint8_t duration);
        void cancelTemp();
        void setBolus(float amount);
        bool stopPump();
        bool startPump();

    private:
        bool pumpOn = true;

        void pressBACK();
        void pressACT();
        void pressESC();
        void pressUP();
        void pressDOWN();
};
#endif