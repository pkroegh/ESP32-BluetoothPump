// Bluetooth library for communication between ESP and AndroidAPS

// ensure this library description is only included once
#ifndef MedtronicBluetooth_h
#define MedtronicBluetooth_h

#include <Arduino.h>
#include <BluetoothSerial.h>

class MedtronicBluetooth {
    public:
        MedtronicBluetooth(String deviceName, bool setDebug = false);

        String getMessage();
        void sendMessage(String message);

    private:
        BluetoothSerial SerialBT;

        bool debugBluetooth = false;

        String readBluetooth(String dataString = "");
        void sendBluetooth(String message);
        void debug(uint8_t debuglvl);
};
#endif