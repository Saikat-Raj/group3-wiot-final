#include <constants.h>
#include <secrets.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <time.h>
#include <SPIFFS.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

// Constants for WiFi data sender
#define RETRY_COUNTER 3
#define ACK_TIMEOUT 5000 // 5 seconds

// Global variables for contact tracking (must be declared before classes)
RTC_DATA_ATTR unsigned long bootCount = 0;
RTC_DATA_ATTR unsigned long lastUploadDuration = 0;

// Persistent contact tracking (limited to 10 devices for memory efficiency)
#define MAX_TRACKED_DEVICES 10
RTC_DATA_ATTR char trackedDevices[MAX_TRACKED_DEVICES][18]; // MAC addresses
RTC_DATA_ATTR unsigned long firstSeenTimes[MAX_TRACKED_DEVICES];
RTC_DATA_ATTR unsigned long closeContactDurations[MAX_TRACKED_DEVICES]; // Cumulative close contact time
RTC_DATA_ATTR unsigned long lastCloseContactTimes[MAX_TRACKED_DEVICES]; // Last time device was in close contact
RTC_DATA_ATTR int trackedDeviceCount = 0;

// Forward declarations for helper functions
int findTrackedDevice(const char* deviceAddress);
unsigned long getFirstSeenTime(const char* deviceAddress);
void addOrUpdateTrackedDevice(const char* deviceAddress, unsigned long currentTime);
unsigned long getCloseContactDuration(const char* deviceAddress);
void updateCloseContact(const char* deviceAddress, unsigned long currentTime, int rssi);
bool isExposureEvent(const char* deviceAddress, unsigned long currentTime);

// WifiDataSender class
class WifiDataSender {
private:
    const char* _ssid;
    const char* _password;
    const char* _udpAddress;
    unsigned int _udpPort;
    WiFiUDP _udp;
    bool _packetAcknowledged;
    uint8_t _retryCounter;
    bool _debug;

    void _connectToWiFi() {
        WiFi.mode(WIFI_STA);
        DEBUG_LOGN("-- LOG: Connecting to WiFi...");
        WiFi.begin(_ssid, _password);

        unsigned long timeout = millis() + 10000; // 10 second timeout

        while (WiFi.status() != WL_CONNECTED && millis() < timeout) {
            delay(100);
            DEBUG_LOG(".");
        }

        if (WiFi.status() != WL_CONNECTED) {
            DEBUG_LOGN("\n-- ERROR: WiFi Connection Failed!!");
            return;
        }

        DEBUG_LOGN("\n-- SUCCESS: WiFi Connected!!");
        DEBUG_LOG("-- LOG: IP Address: ");
        DEBUG_LOGN(WiFi.localIP());

        _udp.begin(_udpPort);
    }

    bool waitForAcknowledgment() {
        unsigned long timeout = millis() + ACK_TIMEOUT;
        
        while (millis() < timeout) {
            int packetSize = _udp.parsePacket();
            if (packetSize > 0) {
                char ackBuffer[4];
                int len = _udp.read(ackBuffer, sizeof(ackBuffer) - 1);
                if (len > 0) {
                    ackBuffer[len] = '\0';
                    if (strcmp(ackBuffer, "ACK") == 0) {
                        return true;
                    }
                }
            }
            delay(10);
        }
        return false;
    }

    bool _sendDataWithConfirmation(const char* data) {
        for (_retryCounter = 1; _retryCounter <= RETRY_COUNTER; _retryCounter++) {
            // Send UDP packet
            _udp.beginPacket(_udpAddress, _udpPort);
            _udp.print(data);
            _udp.endPacket();

            if (_debug) {
                DEBUG_LOG("-- LOG: Sent packet #");
                DEBUG_LOGN(_retryCounter);
            }

            // Wait for acknowledgment
            if (waitForAcknowledgment()) {
                DEBUG_LOGN("-- SUCCESS: ACK Received!!");
                return true;
            }
            
            DEBUG_LOGN("-- LOG: ACK timeout, retrying...");
        }

        DEBUG_LOGN("-- ERROR: Max retries exceeded!!");
        return false;
    }

public:
    WifiDataSender(const char* ssid, const char* password, const char* udpAddress, unsigned int udpPort, bool debug)
        : _ssid(ssid), _password(password), _udpAddress(udpAddress), _udpPort(udpPort),
          _packetAcknowledged(false), _debug(debug), _retryCounter(0) {}

    unsigned long getUnixTime() {
        if (WiFi.status() != WL_CONNECTED) {
            _connectToWiFi();
        }

        configTime(0, 0, TIME_SERVER);

        struct tm timeinfo;
        if (!getLocalTime(&timeinfo)) {
            return 0;
        }
        
        time_t now;
        time(&now);
        return now;
    }

    bool uploadData(const String data) {
        if (WiFi.status() != WL_CONNECTED) {
            _connectToWiFi();
        }

        return _sendDataWithConfirmation(data.c_str());
    }
};

// Data storage functions
void storeData(const char* fileName, const String data) {
    DEBUG_LOGF("-- LOG: Writing file: %s\r\n", fileName);
    File file = SPIFFS.open(fileName, FILE_APPEND);
    if (!file) {
        DEBUG_LOGN("-- ERROR: Failed to open file!");
        return;
    }

    if (file && file.size() == 0) {
        DEBUG_LOGN("-- LOG: Created the Data File!!");
        file.println("timeStamp,peerId,rssi,deviceId,uploadDuration,contactDuration,closeContactDuration,exposureStatus");
    }

    if (!file.println(data)) {
        DEBUG_LOGN("-- ERROR: Write failed!");
    }

    file.close();
}

String readData(const char* fileName) {
    if (!SPIFFS.exists(fileName)) {
        DEBUG_LOGN("-- ERROR: File doesn't exist!");
        return String();
    }

    File file = SPIFFS.open(fileName, "r");
    if (!file) {
        DEBUG_LOGN("-- ERROR: Failed to open file!");
        return String();
    }

    String content = "";
    while (file.available()) {
        content += (char)file.read();
    }
    DEBUG_LOGN("-- LOG: Data:");
    DEBUG_LOGN(content);

    file.close();
    return content;
}

// Utility functions
void checkSPIFFS() {
    DEBUG_LOGN("\n--- SPIFFS Diagnostics ---");
 
    if (!SPIFFS.begin(true)) {
        DEBUG_LOGN("-- ERROR: SPIFFS Mount Failed! Formatting...");
        SPIFFS.format();
        if (!SPIFFS.begin(true)) {
            DEBUG_LOGN("-- ERROR: Format Failed! Halting.");
            while(1);
        }
    }

    DEBUG_LOGF("-- LOG: Total space: %10d bytes\n", SPIFFS.totalBytes());
    DEBUG_LOGF("-- LOG: Used space:  %10d bytes\n", SPIFFS.usedBytes());

    DEBUG_LOGN("\n-- LOG: File List:");
    File root = SPIFFS.open("/");
    File file = root.openNextFile();
    while(file){
        DEBUG_LOGF("-- LOG:   %-20s %8d bytes\n", file.name(), file.size());
        file = root.openNextFile();
    }

    const char* testFile = "./spiffs_test.txt";
    DEBUG_LOGF("\n-- LOG: Testing write to %s...", testFile);

    File test = SPIFFS.open(testFile, FILE_WRITE);
    if(test) {
        test.println("-- SUCCESS: SPIFFS TEST SUCCESS");
        test.close();
        DEBUG_LOGN("OK");

        test = SPIFFS.open(testFile, FILE_READ);
        DEBUG_LOG("-- LOG: Contents: ");
        while(test.available()) Serial.write(test.read());
        test.close();
    } else {
        DEBUG_LOGN("-- ERROR: FAILED");
    }

    SPIFFS.remove(testFile);
    DEBUG_LOGN("--- End Diagnostics ---\n");
}

// BluetoothScanner class
class BluetoothScanner {
private:
    const unsigned long startTime;
    String deviceId;
    unsigned long lastUploadDuration;

    String generateDeviceId() {
        String id = "ESP32_";
        for (int i = 0; i < DEVICE_ID_LENGTH; i++) {
            id += String(random(0, 10));
        }
        return id;
    }

    void updateDeviceId() {
        deviceId = generateDeviceId();
        DEBUG_LOG("Updated Device ID: ");
        DEBUG_LOGN(deviceId);
        
        BLEAdvertising *advertising = BLEDevice::getAdvertising();
        advertising->stop();
        
        BLEAdvertisementData adData;
        adData.setManufacturerData(deviceId);
        advertising->setAdvertisementData(adData);
        
        advertising->start();
    }

    bool isContactTracingDevice(BLEAdvertisedDevice device) {
        String manufacturerData = device.getManufacturerData();

        if (manufacturerData.length() > 0) {
            if (manufacturerData.indexOf("ESP32_") != -1 && manufacturerData != deviceId) {
                return true;
            }
        }

        return false;
    }

    void recordDeviceContact(BLEAdvertisedDevice device) {
        String deviceAddress = device.getAddress().toString();
        int rssi = device.getRSSI();
        
        unsigned long currentTime = startTime + (millis() / 1000);
        
        unsigned long firstSeen = getFirstSeenTime(deviceAddress.c_str());
        if (firstSeen == 0) {
            addOrUpdateTrackedDevice(deviceAddress.c_str(), currentTime);
            firstSeen = currentTime;
            DEBUG_LOG("First contact with device: ");
            DEBUG_LOG(deviceAddress);
            DEBUG_LOG(" at time: ");
            DEBUG_LOGN(currentTime);
        }
        
        updateCloseContact(deviceAddress.c_str(), currentTime, rssi);
        
        unsigned long contactDuration = currentTime - firstSeen;
        unsigned long closeContactDuration = getCloseContactDuration(deviceAddress.c_str());
        
        int index = findTrackedDevice(deviceAddress.c_str());
        if (index >= 0 && lastCloseContactTimes[index] > 0) {
            closeContactDuration += (currentTime - lastCloseContactTimes[index]);
        }
        
        bool isExposure = isExposureEvent(deviceAddress.c_str(), currentTime);
        String exposureStatus = isExposure ? "EXPOSURE" : "NORMAL";
        
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

    void processScannedDevice(BLEAdvertisedDevice device) {
        if (isContactTracingDevice(device) && device.getRSSI() >= MIN_RSSI) {
            DEBUG_LOG("Found contact tracing device: ");
            DEBUG_LOGN(device.getAddress().toString());
            DEBUG_LOG("RSSI: ");
            DEBUG_LOGN(device.getRSSI());
            recordDeviceContact(device);
        }
    }

public:
    BluetoothScanner(unsigned long unixTime, unsigned long uploadDuration = 0)
        : startTime(unixTime), lastUploadDuration(uploadDuration) {
        randomSeed(micros());
        deviceId = generateDeviceId();
        DEBUG_LOG("Generated Device ID: ");
        DEBUG_LOGN(deviceId);
    }

    void initBluetooth() {
        BLEDevice::init(BLE_DEVICE_NAME);
        BLEServer *bleServer = BLEDevice::createServer();

        DEBUG_LOGN("LOG: Started Bluetooth Server!!");

        BLEService *bleService = bleServer->createService(SERVICE_UUID);
        BLECharacteristic *bleCharacteristic = bleService->createCharacteristic(CHARACTERISTIC_UUID, BLECharacteristic::PROPERTY_READ);
        bleCharacteristic->setValue(CHARACTERISTIC_VALUE);
        bleService->start();

        BLEAdvertising *advertising = BLEDevice::getAdvertising();

        BLEAdvertisementData adData;
        adData.setManufacturerData(deviceId);
        advertising->setAdvertisementData(adData);
        advertising->addServiceUUID(SERVICE_UUID);
        advertising->setScanResponse(true);
        advertising->setMinPreferred(0x06);
        advertising->setMinPreferred(0x12);
        advertising->start();
    }

    void performScan() {
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
        updateDeviceId();
    }
};

// Contact tracking helper functions
int findTrackedDevice(const char* deviceAddress) {
  for (int i = 0; i < trackedDeviceCount; i++) {
    if (strcmp(trackedDevices[i], deviceAddress) == 0) {
      return i;
    }
  }
  return -1;
}

unsigned long getFirstSeenTime(const char* deviceAddress) {
  int index = findTrackedDevice(deviceAddress);
  return (index >= 0) ? firstSeenTimes[index] : 0;
}

void addOrUpdateTrackedDevice(const char* deviceAddress, unsigned long currentTime) {
  if (findTrackedDevice(deviceAddress) >= 0) {
    return; // Already tracking this device
  }
  
  if (trackedDeviceCount < MAX_TRACKED_DEVICES) {
    int newIndex = trackedDeviceCount++;
    strcpy(trackedDevices[newIndex], deviceAddress);
    firstSeenTimes[newIndex] = currentTime;
    closeContactDurations[newIndex] = 0;
    lastCloseContactTimes[newIndex] = 0;
    
    DEBUG_LOG("Started tracking device: ");
    DEBUG_LOGN(deviceAddress);
  }
}

unsigned long getCloseContactDuration(const char* deviceAddress) {
  int index = findTrackedDevice(deviceAddress);
  return (index >= 0) ? closeContactDurations[index] : 0;
}

void updateCloseContact(const char* deviceAddress, unsigned long currentTime, int rssi) {
  int index = findTrackedDevice(deviceAddress);
  if (index < 0) return;
  
  bool isCloseContact = (rssi >= CLOSE_CONTACT_RSSI);
  bool wasInCloseContact = (lastCloseContactTimes[index] > 0);
  
  if (isCloseContact && !wasInCloseContact) {
    // Starting new close contact period
    lastCloseContactTimes[index] = currentTime;
  } else if (!isCloseContact && wasInCloseContact) {
    // Ending close contact period
    unsigned long contactDuration = currentTime - lastCloseContactTimes[index];
    closeContactDurations[index] += contactDuration;
    lastCloseContactTimes[index] = 0;
    
    DEBUG_LOG("Close contact ended. Added ");
    DEBUG_LOG(contactDuration);
    DEBUG_LOG(" seconds to device ");
    DEBUG_LOGN(deviceAddress);
  }
}

bool isExposureEvent(const char* deviceAddress, unsigned long currentTime) {
  int index = findTrackedDevice(deviceAddress);
  if (index < 0) return false;
  
  unsigned long totalCloseContactTime = closeContactDurations[index];
  
  // Include ongoing close contact time
  if (lastCloseContactTimes[index] > 0) {
    totalCloseContactTime += (currentTime - lastCloseContactTimes[index]);
  }
  
  return totalCloseContactTime >= EXPOSURE_TIME_THRESHOLD;
}

void setup() {
  Serial.begin(115200);
  delay(100);

  initializeStorage();
  
  WifiDataSender wifiSender = WifiDataSender(SSID, PASSWORD, UDP_ADDRESS, UDP_PORT, true);
  unsigned long currentTime = wifiSender.getUnixTime();
  
  logBootInfo();
  
  performBluetoothScan(currentTime);
  
  uploadDataIfNeeded(wifiSender, currentTime);

  enterDeepSleep();
}

void initializeStorage() {
  if (!SPIFFS.begin(true)) {
    DEBUG_LOGN("-- ERROR: SPIFFS Mount Failed!");
  }

  if (bootCount == 0) {
    SPIFFS.format();
    checkSPIFFS();
  }
}

void logBootInfo() {
  DEBUG_LOG("-- LOG: Boot count: ");
  DEBUG_LOG(bootCount);
  DEBUG_LOG(", Last upload duration: ");
  DEBUG_LOG(lastUploadDuration);
  DEBUG_LOGN(" ms");
}

void performBluetoothScan(unsigned long currentTime) {
  BluetoothScanner scanner = BluetoothScanner(currentTime, lastUploadDuration);
  scanner.initBluetooth();
  scanner.performScan();
}

void uploadDataIfNeeded(WifiDataSender& wifiSender, unsigned long currentTime) {
  if (bootCount > 0 && bootCount % 5 == 0) {
    String data = readData(DATA_FILE);
    unsigned long uploadStart = millis();
    
    String uploadInfo = "# Upload Timestamp: " + String(currentTime) + "\n";
    String dataWithMetadata = uploadInfo + data;
    
    if (wifiSender.uploadData(dataWithMetadata)) {
      lastUploadDuration = millis() - uploadStart;
      DEBUG_LOG("-- LOG: Upload completed in ");
      DEBUG_LOG(lastUploadDuration);
      DEBUG_LOGN(" ms");
      
      SPIFFS.format();
    } else {
      DEBUG_LOGN("-- ERROR: Upload failed");
    }
  }
}

void enterDeepSleep() {
  bootCount++;
  esp_sleep_enable_timer_wakeup(SLEEP_TIME_SECONDS * SECONDS_TO_MICROSECONDS);
  DEBUG_LOGN("-- LOG: Entering deep sleep for " + String(SLEEP_TIME_SECONDS) + " seconds");
  delay(100);
  esp_deep_sleep_start();
}

void loop() {}