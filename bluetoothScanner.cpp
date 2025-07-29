#include <bluetoothScanner.h>

BluetoothScanner::BluetoothScanner(unsigned long unixTime)
    : _unixTime(unixTime) {}

bool BluetoothScanner::_isARelevantDevice(BLEAdvertisedDevice device) {
    String mfgData = device.getManufacturerData();

    if ((mfgData.indexOf("ESP32_A") != -1 && String(DEVICE_ID) == "ESP32_B") ||
        (mfgData.indexOf("ESP32_B") != -1 && String(DEVICE_ID) == "ESP32_A")) {
        return true;
    }

    return false;
}

void BluetoothScanner::_addDevice(BLEAdvertisedDevice device) {
    String data = String(_unixTime) + "," + device.getAddress().toString() + "," + String(device.getRSSI());
    storeData(FILE_NAME, data);
}

void BluetoothScanner::_processDevice(BLEAdvertisedDevice device) {
    if (_isARelevantDevice(device)) {
        int rssi = device.getRSSI();

        if (rssi < RSSI_THRESHOLD) {
            DEBUG_LOG("Ignored device due to weak RSSI: ");
            DEBUG_LOGN(rssi);
            return;
        }

        DEBUG_LOG("Found a relevant device with address: ");
        DEBUG_LOGN(device.getAddress().toString());
        DEBUG_LOG("RSSI: ");
        DEBUG_LOGN(rssi);

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

    // Use DEVICE_ID as identifier
    char randomId[ID_SIZE + 1];
    const char charset[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

    for (int i = 0; i < ID_SIZE; i++) {
        randomId[i] = charset[esp_random() % (sizeof(charset) - 1)];
    }
    randomId[ID_SIZE] = '\0';

    DEBUG_LOG("Generated ID: ");
    DEBUG_LOGN(randomId);

    BLEAdvertisementData adData;
    adData.setManufacturerData(String(randomId));
    
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