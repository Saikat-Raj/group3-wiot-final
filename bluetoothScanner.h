
#ifndef BLUETOOTHSCANNER_H
#define BLUETOOTHSCANNER_H

#include <constants.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <storeData.h>

class BluetoothScanner {
private:
    bool _isARelevantDevice(BLEAdvertisedDevice device);
    void _addDevice(BLEAdvertisedDevice device);
    void _processDevice(BLEAdvertisedDevice device);
    const unsigned long _unixTime;
public:
    BluetoothScanner(unsigned long unixTime);
    void initBluetooth();
    void performBLEScan();
};

#endif

