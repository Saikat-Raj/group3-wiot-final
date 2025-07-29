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

void BluetoothScanner::_rotateDeviceId() {
    _deviceId = _generateRandomDeviceId();
    DEBUG_LOG("Rotated to new Device ID: ");
    DEBUG_LOGN(_deviceId);
    
    // Update BLE advertising with new ID
    BLEAdvertising *bleAdvertising = BLEDevice::getAdvertising();
    bleAdvertising->stop();
    
    String mfgDataString = _deviceId;
    BLEAdvertisementData adData;
    adData.setManufacturerData(mfgDataString);
    bleAdvertising->setAdvertisementData(adData);
    
    bleAdvertising->start();
}

bool BluetoothScanner::_isARelevantDevice(BLEAdvertisedDevice device) {
    String mfgData = device.getManufacturerData();

    if (mfgData.length() > 0) {
        // DEBUG_LOG("Checking device with mfg data: ");
        // DEBUG_LOG(mfgData);
        // DEBUG_LOG(" against our ID: ");
        // DEBUG_LOGN(_deviceId);
        
        if (mfgData.indexOf("ESP32_") != -1 && mfgData != _deviceId) {
            return true;
        }
    }

    return false;
}

// Forward declarations for global functions
extern void addOrUpdateTrackedDevice(const char* deviceAddress, unsigned long currentTime);
extern unsigned long getFirstSeenTime(const char* deviceAddress);
extern unsigned long getCloseContactDuration(const char* deviceAddress);
extern void updateCloseContact(const char* deviceAddress, unsigned long currentTime, int rssi);
extern bool isExposureEvent(const char* deviceAddress, unsigned long currentTime);

void BluetoothScanner::_addDevice(BLEAdvertisedDevice device) {
    String deviceAddress = device.getAddress().toString();
    int rssi = device.getRSSI();
    
    // Get current time (boot time + elapsed seconds)
    unsigned long currentTime = _unixTime + (millis() / 1000);
    
    // Use persistent tracking across boot cycles
    unsigned long firstSeen = getFirstSeenTime(deviceAddress.c_str());
    if (firstSeen == 0) {
        // First time seeing this device
        addOrUpdateTrackedDevice(deviceAddress.c_str(), currentTime);
        firstSeen = currentTime;
        DEBUG_LOG("First contact with device: ");
        DEBUG_LOG(deviceAddress);
        DEBUG_LOG(" at time: ");
        DEBUG_LOGN(currentTime);
    }
    
    // Update close contact tracking based on RSSI
    updateCloseContact(deviceAddress.c_str(), currentTime, rssi);
    
    // Calculate total contact duration and close contact duration
    unsigned long contactDuration = currentTime - firstSeen;
    unsigned long closeContactDuration = getCloseContactDuration(deviceAddress.c_str());
    
    // Evaluate exposure status
    bool isExposure = isExposureEvent(deviceAddress.c_str(), currentTime);
    String exposureStatus = isExposure ? "EXPOSURE" : "NORMAL";
    
    // Enhanced debug logging
    DEBUG_LOG("Device: ");
    DEBUG_LOG(deviceAddress);
    DEBUG_LOG(" RSSI: ");
    DEBUG_LOG(rssi);
    DEBUG_LOG(" Total: ");
    DEBUG_LOG(contactDuration);
    DEBUG_LOG("s Close: ");
    DEBUG_LOG(closeContactDuration);
    DEBUG_LOG("s Status: ");
    DEBUG_LOGN(exposureStatus);
    
    if (isExposure) {
        DEBUG_LOG("*** EXPOSURE EVENT DETECTED with ");
        DEBUG_LOG(deviceAddress);
        DEBUG_LOG(" (");
        DEBUG_LOG(closeContactDuration);
        DEBUG_LOGN(" seconds close contact) ***");
    }
    
    String data = String(currentTime) + "," + deviceAddress + "," + String(rssi) + "," + _deviceId + "," + String(_uploadDuration) + "," + String(contactDuration) + "," + String(closeContactDuration) + "," + exposureStatus;
    storeData(FILE_NAME, data);
}

void BluetoothScanner::_processDevice(BLEAdvertisedDevice device) {
    if (_isARelevantDevice(device) && device.getRSSI() >= RSSI_THRESHOLD) {
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
    
    // Rotate device ID after each scan for runtime privacy
    _rotateDeviceId();
}