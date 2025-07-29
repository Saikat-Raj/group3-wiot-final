
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

// Helper functions for persistent contact tracking
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
  if (index >= 0) {
    return firstSeenTimes[index];
  }
  return 0;
}

void addOrUpdateTrackedDevice(const char* deviceAddress, unsigned long currentTime) {
  int index = findTrackedDevice(deviceAddress);
  if (index >= 0) {
    // Device already tracked, no need to update first seen time
    return;
  }
  
  // Add new device if we have space
  if (trackedDeviceCount < MAX_TRACKED_DEVICES) {
    strcpy(trackedDevices[trackedDeviceCount], deviceAddress);
    firstSeenTimes[trackedDeviceCount] = currentTime;
    closeContactDurations[trackedDeviceCount] = 0;
    lastCloseContactTimes[trackedDeviceCount] = 0;
    trackedDeviceCount++;
    DEBUG_LOG("Added device to persistent tracking: ");
    DEBUG_LOGN(deviceAddress);
  }
}

unsigned long getCloseContactDuration(const char* deviceAddress) {
  int index = findTrackedDevice(deviceAddress);
  if (index >= 0) {
    return closeContactDurations[index];
  }
  return 0;
}

void updateCloseContact(const char* deviceAddress, unsigned long currentTime, int rssi) {
  int index = findTrackedDevice(deviceAddress);
  if (index < 0) return;
  
  if (rssi >= CLOSE_CONTACT_RSSI) {
    // Device is in close contact range
    if (lastCloseContactTimes[index] == 0) {
      // Start of new close contact period
      lastCloseContactTimes[index] = currentTime;
    }
    // Close contact is ongoing, duration will be calculated when contact ends or in evaluation
  } else {
    // Device is no longer in close contact range
    if (lastCloseContactTimes[index] > 0) {
      // End of close contact period, add duration
      unsigned long contactPeriod = currentTime - lastCloseContactTimes[index];
      closeContactDurations[index] += contactPeriod;
      lastCloseContactTimes[index] = 0;
      DEBUG_LOG("Close contact ended. Added ");
      DEBUG_LOG(contactPeriod);
      DEBUG_LOG(" seconds to device ");
      DEBUG_LOGN(deviceAddress);
    }
  }
}

bool isExposureEvent(const char* deviceAddress, unsigned long currentTime) {
  int index = findTrackedDevice(deviceAddress);
  if (index < 0) return false;
  
  unsigned long totalCloseContact = closeContactDurations[index];
  
  // Add current ongoing close contact time if applicable
  if (lastCloseContactTimes[index] > 0) {
    totalCloseContact += (currentTime - lastCloseContactTimes[index]);
  }
  
  return totalCloseContact >= EXPOSURE_DURATION_THRESHOLD;
}

void setup() {
  Serial.begin(115200);
  delay(100);

  if (!SPIFFS.begin(true)) {
    DEBUG_LOGN("-- ERROR: SPIFFS Mount Failed!");
  }

  if (bootCount == 0) {
    SPIFFS.format();
    checkSPIFFS();
  }

  WifiDataSender wifiDataSender = WifiDataSender(SSID, PASSWORD, UDP_ADDRESS, UDP_PORT, true);
  unsigned long unixTime = wifiDataSender.getUnixTime();
  
  DEBUG_LOG("-- LOG: Boot count: ");
  DEBUG_LOG(bootCount);
  DEBUG_LOG(", Last upload duration: ");
  DEBUG_LOG(lastUploadDuration);
  DEBUG_LOGN(" ms");
  
  BluetoothScanner bluetoothScanner = BluetoothScanner(unixTime, lastUploadDuration);

  bluetoothScanner.initBluetooth();
  bluetoothScanner.performBLEScan();

  if (bootCount > 0 && bootCount % 5 == 0) {
    String data = readData(FILE_NAME);
    unsigned long uploadStartTime = millis();
    
    // Add upload metadata header
    String uploadInfo = "# Upload Timestamp: " + String(unixTime) + "\n";
    String dataWithMetadata = uploadInfo + data;
    
    if (wifiDataSender.uploadData(dataWithMetadata)) {
      unsigned long uploadDuration = millis() - uploadStartTime;
      lastUploadDuration = uploadDuration;
      DEBUG_LOG("-- LOG: Upload completed in ");
      DEBUG_LOG(uploadDuration);
      DEBUG_LOG(" ms. Stored for next cycle: ");
      DEBUG_LOGN(lastUploadDuration);
      
      SPIFFS.format();
    } else {
      DEBUG_LOGN("-- ERROR: Upload failed, duration not recorded");
    }
  }

  bootCount++;
  esp_sleep_enable_timer_wakeup(DEEP_SLEEP_TIME * MICRO_SECOND_TO_SECOND);
  DEBUG_LOGN("-- LOG: Entering deep sleep for " + String(DEEP_SLEEP_TIME) + " seconds");
  delay(100);
  esp_deep_sleep_start();
}

void loop() {}

