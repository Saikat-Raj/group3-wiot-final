#include "wifiDataSender.h"

#define RETRY_COUNTER 3
#define ACK_TIMEOUT 5000 // 1 second
#define MICRO_SECOND_TO_SECOND 1000000
#define DEEP_SLEEP_TIME 15 // Deep Sleep for 15 seconds

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

    unsigned long start = millis();

    // Retry connecting till we connect or exceed time limit
    while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) {
        delay(100);
        DEBUG_LOG(".");
    }

    if (WiFi.status() != WL_CONNECTED) {
        DEBUG_LOGN("\n-- ERROR: WiFi Connection Failed!!");
        return;
    }

    DEBUG_LOGN("\n-- SUCCESS: WiFi Connection Successfull!!");
    DEBUG_LOG("-- LOG: Device IP Address: ");
    DEBUG_LOGN(WiFi.localIP());

    // Initialize UDP once WiFi is connected
    _udp.begin(_udpPort);
}

bool WifiDataSender::_sendDataWithConfirmation(const char* data) {
    _packetAcknowledged = false;
    _retryCounter = 0;
    unsigned long lastAttemptTime;

    while (_retryCounter < RETRY_COUNTER && !_packetAcknowledged) {
        _retryCounter++;
        lastAttemptTime = millis();

        // Send UDP Packet
        _udp.beginPacket(_udpAddress, _udpPort);
        _udp.print(data);
        _udp.endPacket();

        if (_debug) {
            DEBUG_LOG("-- LOG: Sent Packet #");
            DEBUG_LOG(_retryCounter);
            DEBUG_LOG(": \n");
            DEBUG_LOGN(data);
        }

        // Wait for ACK message from server with timeout
        while (millis() - lastAttemptTime < ACK_TIMEOUT) {
            int packetSize = _udp.parsePacket();
            if (packetSize) {
                char ackBuffer[4];
                int len = _udp.read(ackBuffer, sizeof(ackBuffer) - 1);
                if (len > 0) {
                    ackBuffer[len] = '\0';
                    if (strcmp(ackBuffer, "ACK") == 0) {
                        _packetAcknowledged = true;
                        DEBUG_LOGN("-- SUCCESS: ACK Received!!");
                        break;
                    }
                }
            }
            delay(10); // Wait a bit before rechecking
        }
        if (!_packetAcknowledged) {
            DEBUG_LOGN("-- LOG: ACK timeout, retrying...");
        }
    }

    if (!_packetAcknowledged) {
        DEBUG_LOGN("-- ERROR: Max Retries Exceeded!!");
        return false;
    } else {
        return true;
    }
}

unsigned long WifiDataSender::getUnixTime() {

    if (WiFi.status() != WL_CONNECTED) {
        _connectToWiFi();
    }

    configTime(0, 0, NTP_SERVER);

    time_t now;
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        return(0);
    }
    time(&now);
    return now;
}

bool WifiDataSender::uploadData(const String data) {
    if (WiFi.status() != WL_CONNECTED) {
        _connectToWiFi();
    }

    // This is temporary but in reality the data uploaded should be 
    // the one that is passed
    // String data = "This is a message " + String(random(0, 100));
    return _sendDataWithConfirmation(data.c_str());
}
