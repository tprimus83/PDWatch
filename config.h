#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <EEPROM.h>
    //Constants
#define MAX_CONFIG_SIZE 10
#define MAX_WIFI_NAME_LENGTH 20
const int ARRAY_START_ADDRESS = sizeof(int);

void readConfig();
void writeConfig();

struct WatchConfig {
    char name[MAX_WIFI_NAME_LENGTH]; // Correct array declaration
    char pwd[MAX_WIFI_NAME_LENGTH]; // Correct array declaration

    // Constructor to initialize default values (if needed)
    WatchConfig() {
        name[0] = '\0'; // Initialize name as an empty string
        pwd[0] =  '\0'; // Initialize name as an empty string
    }
};
WatchConfig* getCurrentConfig();
int getCurrentIndex();
void setCurrentIndex(int index);
WatchConfig getConfigAt(int index);
#endif