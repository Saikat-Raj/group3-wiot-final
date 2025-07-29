#ifndef STORE_DATA_H
#define STORE_DATA_H

#include <SPIFFS.h>
#include <constants.h>

void storeData(const char* fileName, const String data);
String readData(const char* fileName);

#endif // !STORE_DATA_H
