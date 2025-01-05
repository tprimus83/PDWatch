#include "config.h"

WatchConfig configurations[MAX_CONFIG_SIZE];

int configIndex = 0;

// Function to write a WatchConfig array to EEPROM
void writeConfig() {
    EEPROM.begin(512); // Initialize EEPROM with 512 bytes size (adjust according to your EEPROM size)
    EEPROM.put(0, configIndex);
    for (int i = 0; i < MAX_CONFIG_SIZE; i++) {
        EEPROM.put(ARRAY_START_ADDRESS + i * sizeof(WatchConfig), configurations[i]);
    }
    EEPROM.end();
}

// Function to read a WatchConfig array from EEPROM
void readConfig() {
    EEPROM.begin(512); // Initialize EEPROM with 512 bytes size (adjust according to your EEPROM size)
    EEPROM.get(0, configIndex);
    for (int i = 0; i < MAX_CONFIG_SIZE; i++) {
        EEPROM.get(ARRAY_START_ADDRESS + i * sizeof(WatchConfig), configurations[i]);
    }
    EEPROM.end();

    if (configurations[0].name[0] == '\0') {
        // Initialize array with some data
        configIndex = 0;
        for (int i = 0; i < MAX_CONFIG_SIZE; i++) {
            String("Wifi " + String(i + 1)).toCharArray(configurations[i].name, MAX_WIFI_NAME_LENGTH);
            String("***");
        }

        // Write the array to EEPROM
        writeConfig();
    }
}

WatchConfig* getCurrentConfig() {
    return &(configurations[configIndex]);
}
WatchConfig getConfigAt(int index) {
    return configurations[index];
}

int getCurrentIndex() {
    return configIndex;
}
void setCurrentIndex(int index) {
    configIndex = index;
}