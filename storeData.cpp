#include <storeData.h>

void storeData(const char* fileName, const String data) {
    DEBUG_LOGF("-- LOG: Writing file: %s\r\n", fileName);
    File file = SPIFFS.open(fileName, FILE_APPEND);
    if (!file) {
        DEBUG_LOGN("-- ERROR: Failed to open file!");
        return;
    }

    if (file && file.size() == 0) {
        // DONT CHANGE THIS
        // It matches the 45 length that we need
        DEBUG_LOGN("-- LOG: Created the Data File!!");
        file.println("timeStamp,peerId,rssi,deviceId,uploadDuration,contactDuration,closeContactDuration,exposureStatus");
    }

    // Write to file
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
