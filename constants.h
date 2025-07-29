
#ifndef CONSTANTS_H
#define CONSTANTS_H

#define MICRO_SECOND_TO_SECOND 1000000
#define MILLI_SECOND_TO_SECOND 1000
#define DEEP_SLEEP_TIME 10
#define FILE_NAME "/data.csv"
#define SCAN_TIME 15
#define RSSI_THRESHOLD -90

#define BLE_DEVICE_NAME "ESP32_ContactTracer"
#define SERVICE_UUID "12345678-1234-5678-1234-56789abcdef0"
#define CHARACTERISTIC_UUID "abcdefab-1234-5678-1234-abcdefabcdef"
#define CHARACTERISTIC_VALUE "Hello"
#define COMPANY_ID 0x1234 // Just a dummy manufacturer ID
#define ID_SIZE 8
#define NTP_SERVER "pool.ntp.org"

// ⚠ CHANGE THIS PER DEVICE
#define DEVICE_ID "ESP32_B" // or "ESP32_B"


// Set to 0 in production 
#define DEBUG_MODE 1

#if DEBUG_MODE
    #define DEBUG_LOGF(...) Serial.printf(__VA_ARGS__)
    #define DEBUG_LOG(...) Serial.print(__VA_ARGS__)
    #define DEBUG_LOGN(...) Serial.println(__VA_ARGS__)
#else
    #define DEBUG_LOGF(...)
    #define DEBUG_LOG(...)
    #define DEBUG_LOGN(...)
#endif

#endif // !CONSTANTS_H



