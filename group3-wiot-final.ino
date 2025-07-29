
#include <constants.h>
#include <secrets.h>
#include <wifiDataSender.h>
#include <storeData.h>
#include <bluetoothScanner.h>
#include <utils.h>

RTC_DATA_ATTR unsigned long bootCount = 0;
RTC_DATA_ATTR unsigned long lastUploadDuration = 0;

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

