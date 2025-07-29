
#include <constants.h>
#include <secrets.h>
#include <wifiDataSender.h>
#include <storeData.h>
#include <bluetoothScanner.h>
#include <utils.h>

RTC_DATA_ATTR unsigned long bootCount = 0;
RTC_DATA_ATTR unsigned long lastUploadDuration = 0;

// Persistent contact tracking (limited to 10 devices for memory efficiency)
#define MAX_TRACKED_DEVICES 10
RTC_DATA_ATTR char trackedDevices[MAX_TRACKED_DEVICES][18]; // MAC addresses
RTC_DATA_ATTR unsigned long firstSeenTimes[MAX_TRACKED_DEVICES];
RTC_DATA_ATTR unsigned long closeContactDurations[MAX_TRACKED_DEVICES]; // Cumulative close contact time
RTC_DATA_ATTR unsigned long lastCloseContactTimes[MAX_TRACKED_DEVICES]; // Last time device was in close contact
RTC_DATA_ATTR int trackedDeviceCount = 0;

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
