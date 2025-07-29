#include <bluetoothScanner.h>

BluetoothScanner::BluetoothScanner(unsigned long unixTime, unsigned long uploadDuration)
    : _unixTime(unixTime), _uploadDuration(uploadDuration) {
    randomSeed(micros());
    _deviceId = _generateRandomDeviceId();
    DEBUG_LOG("Generated Device ID: ");
    DEBUG_LOGN(_deviceId);
}

String BluetoothScanner::_generateRandomDeviceId() {
    String deviceId = "ESP32_";
    for (int i = 0; i < ID_SIZE; i++) {
        deviceId += String(random(0, 10));
    }
    return deviceId;
}

bool BluetoothScanner::_isARelevantDevice(BLEAdvertisedDevice device) {
    String mfgData = device.getManufacturerData();

    if (mfgData.length() > 0) {
        DEBUG_LOG("Checking device with mfg data: ");
        DEBUG_LOG(mfgData);
        DEBUG_LOG(" against our ID: ");
        DEBUG_LOGN(_deviceId);
        
        if (mfgData.indexOf("ESP32_") != -1 && mfgData != _deviceId) {
            return true;
        }
    }

    return false;
}

void BluetoothScanner::_addDevice(BLEAdvertisedDevice device) {
    String data = String(_unixTime) + "," + device.getAddress().toString() + "," + String(device.getRSSI()) + "," + _deviceId + "," + String(_uploadDuration);
    storeData(FILE_NAME, data);
}

void BluetoothScanner::_processDevice(BLEAdvertisedDevice device) {
    if (_isARelevantDevice(device)) {
        DEBUG_LOG("Found a relevant device with address: ");
        DEBUG_LOGN(device.getAddress().toString());
        DEBUG_LOG("RSSI: ");
        DEBUG_LOGN(device.getRSSI());
        _addDevice(device);
    }
}

void BluetoothScanner::initBluetooth() {
    BLEDevice::init(BLE_DEVICE_NAME);
    BLEServer *bleServer = BLEDevice::createServer();

    DEBUG_LOGN("LOG: Started Bluetooth Server!!");

    BLEService *bleService = bleServer->createService(SERVICE_UUID);
    BLECharacteristic *bleCharacteristic = bleService->createCharacteristic(CHARACTERISTIC_UUID, BLECharacteristic::PROPERTY_READ);
    bleCharacteristic->setValue(CHARACTERISTIC_VALUE);
    bleService->start();

    BLEAdvertising *bleAdvertising = BLEDevice::getAdvertising();

    // Use generated device ID as identifier
    String mfgDataString = _deviceId;
    BLEAdvertisementData adData;
    adData.setManufacturerData(mfgDataString);
    bleAdvertising->setAdvertisementData(adData);
    bleAdvertising->addServiceUUID(SERVICE_UUID);
    bleAdvertising->setScanResponse(true);
    bleAdvertising->setMinPreferred(0x06);
    bleAdvertising->setMinPreferred(0x12);
    bleAdvertising->start();
}

void BluetoothScanner::performBLEScan() {
    BLEScan *bleScanner = BLEDevice::getScan();
    bleScanner->setActiveScan(true);
    bleScanner->setWindow(99);

    BLEScanResults *foundDevices = bleScanner->start(SCAN_TIME, false);
    int numberOfFoundDevices = foundDevices->getCount();

    DEBUG_LOG("Devices Found: ");
    DEBUG_LOGN(numberOfFoundDevices);
    DEBUG_LOGN("Scan Done!");

    for (int i = 0; i < numberOfFoundDevices; i++) {
        BLEAdvertisedDevice d = foundDevices->getDevice(i);
        _processDevice(d);
    }

    bleScanner->clearResults();
}