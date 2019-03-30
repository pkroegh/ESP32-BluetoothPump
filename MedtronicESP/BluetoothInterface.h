// ESP <-> AndroidAPS interface
// Bluetooth library for communication between ESP and AndroidAPS

// ensure this library description is only included once
#ifndef BluetoothInterface_h
#define BluetoothInterface_h

#include <Arduino.h>
#include <BluetoothSerial.h>

class BluetoothInterface {
    public:
        BluetoothInterface(void);

        void begin(String name);
        void begin(String name, bool debug);
        String getMessage();
        void sendMessage(String message);
    private:
        BluetoothSerial SerialBT;
        bool _debug = false;
        String _deviceName;

        String readBluetooth(String dataString = "");
        void sendBluetooth(String message);
};
#endif