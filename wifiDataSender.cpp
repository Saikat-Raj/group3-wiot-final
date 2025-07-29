#include "wifiDataSender.h"

#define RETRY_COUNTER 3
#define ACK_TIMEOUT 5000 // 1 second

WifiDataSender::WifiDataSender(const char* ssid, const char* password, const char* udpAddress, unsigned int udpPort, bool debug)
    : _ssid(ssid),
      _password(password),
      _udpAddress(udpAddress),
      _udpPort(udpPort),
      _packetAcknowledged(false),
      _debug(debug),
      _retryCounter(0) {}

void WifiDataSender::_connectToWiFi() {
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

bool WifiDataSender::_sendDataWithConfirmation(const char* data) {
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

bool WifiDataSender::waitForAcknowledgment() {
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

unsigned long WifiDataSender::getUnixTime() {
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

bool WifiDataSender::uploadData(const String data) {
    if (WiFi.status() != WL_CONNECTED) {
        _connectToWiFi();
    }

    return _sendDataWithConfirmation(data.c_str());
}
