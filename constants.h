
#ifndef CONSTANTS_H
#define CONSTANTS_H

// Time conversions
#define SECONDS_TO_MICROSECONDS 1000000
#define SLEEP_TIME_SECONDS 5

// File storage
#define DATA_FILE "/data.csv"

// Bluetooth scanning
#define SCAN_DURATION 10
#define MIN_RSSI -100
#define CLOSE_CONTACT_RSSI -60    // ~1.5m distance
#define EXPOSURE_TIME_THRESHOLD 300  // 5 minutes

// Bluetooth configuration
#define BLE_DEVICE_NAME "ESP32_ContactTracer"
#define SERVICE_UUID "12345678-1234-5678-1234-56789abcdef0"
#define CHARACTERISTIC_UUID "abcdefab-1234-5678-1234-abcdefabcdef"
#define CHARACTERISTIC_VALUE "Hello"

// Device identification
#define DEVICE_ID_LENGTH 8
#define TIME_SERVER "pool.ntp.org"

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
