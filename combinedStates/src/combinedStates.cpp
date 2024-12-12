#include "Particle.h"
#include "DFRobot_HumanDetection.h"
#include <DFRobot_PN532.h>
#include <Adafruit_MQTT.h>
#include <Adafruit_SSD1306.h>
#include <neopixel.h>
#include "DFRobotDFPlayerMini.h"
#include "IoTClassroom_CNM.h"

// Define constants
#define STATE_ADDRESS 0x00
#define PN532_IRQ 2
#define POLLING 0
#define READ_BLOCK_NO 2
#define PIXELCOUNT 16

// Initialize components
DFRobot_HumanDetection hu(&Serial3);
DFRobot_PN532_IIC nfc(PN532_IRQ, POLLING);
Adafruit_NeoPixel pixel(PIXELCOUNT, SPI1, WS2812B);
Adafruit_SSD1306 display(-1);
DFRobotDFPlayerMini myDFPlayer;

// Global variables
uint8_t dataRead[16] = {0};
enum SensorState { SLEEP_MODE, FALL_MODE };

// Function prototypes
void saveState(SensorState state);
SensorState loadState();
void PixelFill(int startP, int endP, int color);
void displayNFCData();
void MQTT_connect();
bool MQTT_ping();

SYSTEM_MODE(AUTOMATIC);

void setup() {
    Serial.begin(9600);
    waitFor(Serial.isConnected, 10000);
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

    // Initialize other components
    pixel.begin();
    pixel.setBrightness(255);
    pixel.show(); // Turn off all pixels initially

    display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
    display.clearDisplay();
}

void loop() {
    unsigned long currentTime = millis();
    
    // Handle NFC scanning for mode change
    static unsigned long lastNfcScanTime = 0;
    
    if (currentTime - lastNfcScanTime >= 100) { // Check NFC every 100ms
        lastNfcScanTime = currentTime;

        if (nfc.scan()) {
            if (nfc.readData(dataRead, READ_BLOCK_NO) == 1) {
                Serial.printf("NFC Data: %s\n", (char *)dataRead);

                if (strcmp((char *)dataRead, "SLEEP") == 0) {
                    saveState(SLEEP_MODE);
                    System.reset(); // Reset device to apply changes
                } else if (strcmp((char *)dataRead, "FALL") == 0) {
                    saveState(FALL_MODE);
                    System.reset(); // Reset device to apply changes
                }
            }
        }
    }

    // Other tasks...
}

void saveState(SensorState state) {
    EEPROM.write(STATE_ADDRESS, state);
    Serial.printf("State saved: %d\n", state);
}

SensorState loadState() {
    uint8_t storedValue = EEPROM.read(STATE_ADDRESS);  
    SensorState state = static_cast<SensorState>(storedValue); 

    if (state != SLEEP_MODE && state != FALL_MODE) { 
        state = SLEEP_MODE; 
    }

    Serial.printf("State loaded: %d\n", state);
    return state;
}

// Define other functions like PixelFill(), displayNFCData(), MQTT_connect(), etc.

