#include <bluetoothScanner.h>

BluetoothScanner::BluetoothScanner(unsigned long unixTime, unsigned long uploadDuration)
    : startTime(unixTime), lastUploadDuration(uploadDuration) {
    randomSeed(micros());
    deviceId = generateDeviceId();
    DEBUG_LOG("Generated Device ID: ");
    DEBUG_LOGN(deviceId);
}

String BluetoothScanner::generateDeviceId() {
    String id = "ESP32_";
    for (int i = 0; i < DEVICE_ID_LENGTH; i++) {
        id += String(random(0, 10));
    }
    return id;
}

void BluetoothScanner::updateDeviceId() {
    deviceId = generateDeviceId();
    DEBUG_LOG("Updated Device ID: ");
    DEBUG_LOGN(deviceId);
    
    // Update BLE advertising with new ID
    BLEAdvertising *advertising = BLEDevice::getAdvertising();
    advertising->stop();
    
    BLEAdvertisementData adData;
    adData.setManufacturerData(deviceId);
    advertising->setAdvertisementData(adData);
    
    advertising->start();
}

bool BluetoothScanner::isContactTracingDevice(BLEAdvertisedDevice device) {
    String manufacturerData = device.getManufacturerData();

    if (manufacturerData.length() > 0) {
        // Check if it's another ESP32 contact tracer (not ourselves)
        if (manufacturerData.indexOf("ESP32_") != -1 && manufacturerData != deviceId) {
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
extern int findTrackedDevice(const char* deviceAddress);
extern unsigned long lastCloseContactTimes[];

void BluetoothScanner::recordDeviceContact(BLEAdvertisedDevice device) {
    String deviceAddress = device.getAddress().toString();
    int rssi = device.getRSSI();
    
    // Calculate current time from boot
    unsigned long currentTime = startTime + (millis() / 1000);
    
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
    
    // Calculate total close contact duration (including ongoing)
    unsigned long closeContactDuration = getCloseContactDuration(deviceAddress.c_str());
    // Add ongoing close contact time if device is currently in close contact
    int index = findTrackedDevice(deviceAddress.c_str());
    if (index >= 0 && lastCloseContactTimes[index] > 0) {
        closeContactDuration += (currentTime - lastCloseContactTimes[index]);
    }
    
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
    
    String data = String(currentTime) + "," + deviceAddress + "," + String(rssi) + "," + deviceId + "," + String(lastUploadDuration) + "," + String(contactDuration) + "," + String(closeContactDuration) + "," + exposureStatus;
    storeData(DATA_FILE, data);
}

void BluetoothScanner::processScannedDevice(BLEAdvertisedDevice device) {
    if (isContactTracingDevice(device) && device.getRSSI() >= MIN_RSSI) {
        DEBUG_LOG("Found contact tracing device: ");
        DEBUG_LOGN(device.getAddress().toString());
        DEBUG_LOG("RSSI: ");
        DEBUG_LOGN(device.getRSSI());
        recordDeviceContact(device);
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

    BLEAdvertising *advertising = BLEDevice::getAdvertising();

    // Use generated device ID as identifier
    BLEAdvertisementData adData;
    adData.setManufacturerData(deviceId);
    advertising->setAdvertisementData(adData);
    advertising->addServiceUUID(SERVICE_UUID);
    advertising->setScanResponse(true);
    advertising->setMinPreferred(0x06);
    advertising->setMinPreferred(0x12);
    advertising->start();
}

void BluetoothScanner::performScan() {
    BLEScan *scanner = BLEDevice::getScan();
    scanner->setActiveScan(true);
    scanner->setWindow(99);

    BLEScanResults *results = scanner->start(SCAN_DURATION, false);
    int deviceCount = results->getCount();

    DEBUG_LOG("Devices Found: ");
    DEBUG_LOGN(deviceCount);
    DEBUG_LOGN("Scan Complete!");

    for (int i = 0; i < deviceCount; i++) {
        BLEAdvertisedDevice device = results->getDevice(i);
        processScannedDevice(device);
    }

    scanner->clearResults();
    
    // Update device ID after each scan for privacy
    updateDeviceId();
}