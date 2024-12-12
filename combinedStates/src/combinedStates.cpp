#include "Particle.h"
#include "DFRobot_HumanDetection.h"
#include <Adafruit_MQTT.h>
#include "Adafruit_MQTT/Adafruit_MQTT_SPARK.h"
#include <neopixel.h>
#include "colors.h"
#include "Adafruit_SSD1306.h"
#include <DFRobot_PN532.h>
#include "DFRobotDFPlayerMini.h"
#include "credentials.h"

// Conditional compilation to prevent redefinition
#ifndef strncasecmp_P
#define strncasecmp_P(s1, s2, n) strncasecmp((s1), (s2), (n))
#endif

#ifndef strncpy_P
#define strncpy_P(s1, s2, n) strncpy((s1), (s2), (n))
#endif

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
Adafruit_SSD1306 display;
DFRobotDFPlayerMini myDFPlayer;
TCPClient TheClient;
Adafruit_MQTT_SPARK mqtt(&TheClient, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);
Adafruit_MQTT_Publish mmwave = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/mmwave");

// Global variables
uint8_t dataRead[16] = {0};
enum SensorState { SLEEP_MODE, FALL_MODE };
SensorState currentState = SLEEP_MODE; // Default state is Sleep Mode
bool nfcScanned = false;
unsigned int lastTime = 0;
unsigned int lastSecond = 0;
unsigned int lastNfcScanTime = 0;
int movementPixel;
int color;

// Function prototypes (forward declarations)
void saveState(SensorState state);
SensorState loadState();
void configureFallDetection(); // Forward declaration for configureFallDetection
void PixelFill(int startP, int endP, int color); // Forward declaration for PixelFill
void displayNFCData();
void MQTT_connect();
bool MQTT_ping();

SYSTEM_MODE(AUTOMATIC);

void setup() {
    Serial.begin(9600); // Start serial communication
    waitFor(Serial.isConnected, 10000); // Wait for serial connection
    Serial.printf("Starting setup...\n");

    Serial1.begin(9200);
    Serial3.begin(115200);

    // Initialize NFC module
    while (!nfc.begin()) {
        Serial.printf("NFC initialization failed. Retrying...\n");
        delay(1000);
    }
    Serial.printf("NFC initialized successfully.\n");

    // Initialize OLED display
    display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
    display.display(); // Show splash screen
    delay(2000);
    display.clearDisplay(); // Clear screen and buffer

    // Initialize NeoPixels
    pixel.begin();
    pixel.setBrightness(255);
    pixel.show(); // Turn off all pixels initially

    // Connect to WiFi
    WiFi.on();
    WiFi.connect();
    while (WiFi.connecting()) {
        Serial.printf(".");
        delay(500);
    }
    Serial.printf("\nWiFi connected.\n");

    // Initialize mmWave sensor
    Serial.printf("Initializing mmWave sensor...\n");
    while (hu.begin() != 0) {
        Serial.printf("mmWave initialization failed. Retrying...\n");
        delay(1000);
    }
    Serial.printf("mmWave sensor initialized successfully.\n");

    // Load saved mode from EEPROM
    SensorState currentState = loadState();
    Serial.printf("Loaded state: %d\n", currentState);

    // Default to Sleep Mode unless NFC changes it
    if (currentState == SLEEP_MODE || currentState != FALL_MODE) {
        hu.configWorkMode(hu.eSleepMode);
        saveState(SLEEP_MODE); // Save default state
        Serial.printf("Initialized in Sleep Mode.\n");
    } else if (currentState == FALL_MODE) {
        hu.configWorkMode(hu.eFallingMode);
        configureFallDetection();
        Serial.printf("Initialized in Fall Detection Mode.\n");
    }

    hu.sensorRet(); // Reset sensor after configuration

    Serial.printf("Setup complete.\n");
}

void loop() {
    unsigned long currentTime = millis();

    // Debugging: Print loop execution marker
    Serial.printf("Loop running...\n");

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
            } else {
                Serial.printf("Failed to read NFC data.\n");
            }
        }
    }

    // Update NeoPixels based on motion parameters
    movementPixel = hu.smHumanData(hu.eHumanMovingRange);
    
    int pixelNumber = map(movementPixel, 0, 100, 0, PIXELCOUNT - 1);
    
    pixel.clear(); // Clear all pixels first
    PixelFill(0, movementPixel, color);

    // Update OLED every half second
    static unsigned long lastSecond = 0;
    if ((currentTime - lastSecond) > 500) {
        lastSecond = currentTime;

        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(WHITE);
        display.setCursor(0, 0);

        if (currentState == FALL_MODE) {
            display.printf("Fall Status: ");
            switch (hu.getFallData(hu.eFallState)) {
                case 0:
                    display.printf("Not fallen\n");
                    break;
                case 1:
                    display.printf("Fallen\n");
                    break;
                default:
                    display.printf("Read error\n");
            }

            display.printf("Dwell Status: ");
            switch (hu.getFallData(hu.estaticResidencyState)) {
                case 0:
                    display.printf("No dwell\n");
                    break;
                case 1:
                    display.printf("Dwell present\n");
                    break;
                default:
                    display.printf("Read error\n");
            }
        } else {
            display.printf("Movement: %i\nRespiratory rate: %i\nHeart Rate: %i\n",
                           hu.smHumanData(hu.eHumanMovingRange),
                           hu.getBreatheValue(),
                           hu.getHeartRate());
        }

        display.display();
        
        Serial.printf("===============================\n");
    }

    // Publish MQTT data every ten second
    static unsigned long lastTime = 0;
    if ((currentTime - lastTime) > 10000) {
        mmwave.publish(hu.smHumanData(hu.eHumanMovingRange));
        lastTime = currentTime;
        Serial.printf("Published MQTT data.\n");
    }
}

void saveState(SensorState state) {
   EEPROM.write(STATE_ADDRESS, state);
}

SensorState loadState() {
   uint8_t storedValue = EEPROM.read(STATE_ADDRESS);  
   SensorState state = static_cast<SensorState>(storedValue); 
   
   if (state != SLEEP_MODE && state != FALL_MODE) { 
       return SLEEP_MODE; 
   }

   return state;
}

void configureFallDetection() {
   hu.configLEDLight(hu.eFALLLed, 1);         
   hu.configLEDLight(hu.eHPLed, 1);           
   hu.dmInstallHeight(270);                   
   hu.dmFallTime(5);                          
   hu.dmUnmannedTime(1);                      
   hu.dmFallConfig(hu.eResidenceTime, 200);   
   hu.dmFallConfig(hu.eFallSensitivityC, 3);  
}

void PixelFill(int startP, int endP, int color) { 
   endP = constrain(endP, 0, PIXELCOUNT - 1);

   for (movementPixel= startP; movementPixel<= endP; movementPixel++) { 
       pixel.setPixelColor(movementPixel, rainbow[%7]); 
   }

   pixel.show(); 
}
