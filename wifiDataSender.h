#ifndef WIFI_DATA_SENDER_H
#define WIFI_DATA_SENDER_H

#include <constants.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <time.h>

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

        void _connectToWiFi();
        bool _sendDataWithConfirmation(const char* data);

    public:
        WifiDataSender(
            const char* ssid,
            const char* password,
            const char* udpAddress,
            unsigned int udpPort,
            bool debug
        );
        unsigned long getUnixTime();
        bool uploadData(const String data);
};

#endif // WIFI_DATA_SENDER_H
