
#include <constants.h>
#include <secrets.h>
#include <wifiDataSender.h>
#include <storeData.h>
#include <bluetoothScanner.h>
#include <utils.h>

RTC_DATA_ATTR unsigned long bootCount = 0;

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
  BluetoothScanner bluetoothScanner = BluetoothScanner(unixTime);

  bluetoothScanner.initBluetooth();
  bluetoothScanner.performBLEScan();

  if (bootCount > 0 && bootCount % 5 == 0) {
    const String data = readData(FILE_NAME);
    if (wifiDataSender.uploadData(data)) {
      SPIFFS.format();
    }
  }

  bootCount++;
  esp_sleep_enable_timer_wakeup(DEEP_SLEEP_TIME * MICRO_SECOND_TO_SECOND);
  DEBUG_LOGN("-- LOG: Entering deep sleep for " + String(DEEP_SLEEP_TIME) + " seconds");
  delay(100);
  esp_deep_sleep_start();
}

void loop() {}

