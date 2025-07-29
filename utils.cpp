#include <utils.h>

void checkSPIFFS() {
    DEBUG_LOGN("\n--- SPIFFS Diagnostics ---");
 
    // 1. Check mount status
    if (!SPIFFS.begin(true)) {
        DEBUG_LOGN("-- ERROR: SPIFFS Mount Failed! Formatting...");
        SPIFFS.format();
        if (!SPIFFS.begin(true)) {
            DEBUG_LOGN("-- ERROR: Format Failed! Halting.");
            while(1);
        }
    }

    // 2. Print file system info
    DEBUG_LOGF("-- LOG: Total space: %10d bytes\n", SPIFFS.totalBytes());
    DEBUG_LOGF("-- LOG: Used space:  %10d bytes\n", SPIFFS.usedBytes());

    // 3. List all files
    DEBUG_LOGN("\n-- LOG: File List:");
    File root = SPIFFS.open("/");
    File file = root.openNextFile();
    while(file){
        DEBUG_LOGF("-- LOG:   %-20s %8d bytes\n", file.name(), file.size());
        file = root.openNextFile();
    }

    // 4. Test write/read
    const char* testFile = "./spiffs_test.txt";
    DEBUG_LOGF("\n-- LOG: Testing write to %s...", testFile);

    File test = SPIFFS.open(testFile, FILE_WRITE);
    if(test) {
        test.println("-- SUCCESS: SPIFFS TEST SUCCESS");
        test.close();
        DEBUG_LOGN("OK");

        test = SPIFFS.open(testFile, FILE_READ);
        DEBUG_LOG("-- LOG: Contents: ");
        while(test.available()) Serial.write(test.read());
        test.close();
    } else {
        DEBUG_LOGN("-- ERROR: FAILED");
    }

    SPIFFS.remove(testFile);
    DEBUG_LOGN("--- End Diagnostics ---\n");
}
