#include "Particle.h"
#include "IoTClassroom_CNM.h"
#include "DFRobot_HumanDetection.h"
#include <Adafruit_MQTT.h>
#include "Adafruit_MQTT/Adafruit_MQTT_SPARK.h"
#include <neopixel.h>
#include "colors.h"
#include "Adafruit_SSD1306.h"
#include <DFRobot_PN532.h>
#include "DFRobotDFPlayerMini.h"
#include "credentials.h"

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
TCPClient TheClient;
Adafruit_MQTT_SPARK mqtt(&TheClient, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);
Adafruit_MQTT_Publish mmwave = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/mmwave");

// Global variables
uint8_t dataRead[16] = {0};
enum SensorState { SLEEP_MODE, FALL_MODE };
bool nfcScanned = false;
unsigned int lastTime = 0;
unsigned int lastSecond = 0;
unsigned int lastNfcScanTime = 0;
int movementPixel;
int color;

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
    Serial1.begin(9200);
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

    WiFi.on();
    WiFi.connect();
    while (WiFi.connecting()) {
        Serial.printf(".");
    }
}

void loop() {
    unsigned long currentTime = millis();

    // Handle NFC scanning for mode change
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

    // Update NeoPixels based on movement parameter
    movementPixel = hu.smHumanData(hu.eHumanMovingRange);
    int pixelNumber = map(movementPixel, 0, 100, 2, PIXELCOUNT - 1);

    pixel.clear(); // Clear all pixels first
    PixelFill(0, pixelNumber, color);

    // Update OLED every half second
    if ((currentTime - lastSecond) > 500) {
        lastSecond = currentTime;
        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(WHITE);
        display.setCursor(0, 0);
        display.printf("Movement: %i\nRespiratory rate: %i\nHeart Rate: %i\n",
                       hu.smHumanData(hu.eHumanMovingRange),
                       hu.getBreatheValue(),
                       hu.getHeartRate());
        display.display();
    }

    // Publish MQTT data every ten second
    if ((currentTime - lastTime) > 10000) {
        mmwave.publish(hu.smHumanData(hu.eHumanMovingRange));
        lastTime = currentTime;
    }

}

void saveState(SensorState state) {
    EEPROM.write(STATE_ADDRESS, state);
}

SensorState loadState() {
    uint8_t storedValue = EEPROM.read(STATE_ADDRESS);  
    SensorState state = static_cast<SensorState>(storedValue); 

    if (state != SLEEP_MODE && state != FALL_MODE) { 
        state = SLEEP_MODE; 
    }

    return state;
}

void PixelFill(int startP, int endP, int color) {
    endP = constrain(endP, 0, PIXELCOUNT - 1);

    for (int i = startP; i <= endP; i++) {
        pixel.setPixelColor(i, rainbow[i % 7]);
    }
    
    pixel.show();
}

void displayNFCData() {
    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(0, 32);
    display.printf("%s\n", (char *)dataRead);
    display.display();
}

void MQTT_connect() {
   int8_t ret;

   if (mqtt.connected()) {
       return;
   }

   while ((ret = mqtt.connect()) != 0) {
       mqtt.disconnect();
       delay(5000); 
   }
}

bool MQTT_ping() {
   static unsigned int lastPing;
   bool pingStatus;

   if ((millis() - lastPing) > 120000) { 
       pingStatus = mqtt.ping();
       if (!pingStatus) { 
           mqtt.disconnect();
       }
       lastPing = millis();
   }
   
   return pingStatus;
}
