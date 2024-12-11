/* 
 * Project myProject
 * Author: Your Name
 * Date: 
 * For example purposes. Library must be manually put in "lib" folder
 * https://docs.particle.io/firmware/best-practices/firmware-template/
 */

#include "Particle.h"
#include "DFRobot_HumanDetection.h"
#include <DFRobot_PN532.h>

#define STATE_ADDRESS 0x00
#define PN532_IRQ 2
#define POLLING 0
#define READ_BLOCK_NO 2

DFRobot_HumanDetection hu(&Serial3);
DFRobot_PN532_IIC nfc(PN532_IRQ, POLLING);
uint8_t dataRead[16] = {0};

enum SensorState {
    SLEEP_MODE,
    FALL_MODE
};

void saveState(SensorState state) {
    EEPROM.write(STATE_ADDRESS, state);
    Serial.printf("State saved: %d\n", state);
}

SensorState loadState() {
    uint8_t storedValue =EEPROM.read(STATE_ADDRESS);  //read as uint8 
    SensorState state = static_cast<SensorState>(storedValue); //explicit cast convets integer to sensorstate enum
    //state = EEPROM.read(STATE_ADDRESS);
    

    if (state != SLEEP_MODE && state != FALL_MODE) { //Validate state value
        state = SLEEP_MODE; // Default to SLEEP_MODE if invalid
    }

    Serial.printf("State loaded: %d\n", state);
    return state;
}

SYSTEM_MODE(MANUAL);

void setup() {
    Serial.begin(9600);
    waitFor(Serial.isConnected,10000);
    Serial3.begin(115200);

    
    // Initialize NFC module
    while (!nfc.begin()) {
        Serial.println("NFC initialization failed. Retrying...");
        delay(1000);
    }
    
    // Load saved mode from EEPROM and initialize mmWave sensor
    SensorState currentState = loadState();

    if (hu.begin() != 0) {
        Serial.println("mmWave initialization failed!");
        while (true); // Halt execution if initialization fails
    }

    switch (currentState) {
        case SLEEP_MODE:
            hu.configWorkMode(hu.eSleepMode);
            Serial.println("Initialized in Sleep Mode");
            break;

        case FALL_MODE:
            hu.configWorkMode(hu.eFallingMode);
            Serial.println("Initialized in Fall Detection Mode");
            break;
    }

    hu.sensorRet(); // Reset sensor after configuration
}

void loop() {
    
    //Write data to card
    uint8_t blockNumber = 2; //writing to block 2
    uint8_t dataToWrite[] = "FALL";
    if (nfc.writeData(blockNumber, dataToWrite)) {
        Serial.printf ("Data written successfully: %s\n", (char *)dataRead);
    }
    else {
        Serial.printf("Failed to write");
    }
    
    
    
    
    static unsigned long lastNfcScanTime = 0;
    
    if (millis() - lastNfcScanTime >= 100) { // Check NFC every 100ms
        lastNfcScanTime = millis();

        if (nfc.scan()) {
            if (nfc.readData(dataRead, READ_BLOCK_NO) == 1) {
                Serial.printf("NFC Data: %s\n", (char *)dataRead);

                if (strcmp((char *)dataRead, "SLEEP") == 0) {
                    saveState(SLEEP_MODE);
                } else if (strcmp((char *)dataRead, "FALL") == 0) {
                    saveState(FALL_MODE);
                }

                System.reset(); // Reset device to apply changes
            }
        }
    }
}