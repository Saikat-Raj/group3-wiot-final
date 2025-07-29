
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
    bool isContactTracingDevice(BLEAdvertisedDevice device);
    void recordDeviceContact(BLEAdvertisedDevice device);
    void processScannedDevice(BLEAdvertisedDevice device);
    String generateDeviceId();
    void updateDeviceId();
    
    const unsigned long startTime;
    String deviceId;
    unsigned long lastUploadDuration;
    
public:
    BluetoothScanner(unsigned long unixTime, unsigned long uploadDuration = 0);
    void initBluetooth();
    void performScan();
};

#endif
