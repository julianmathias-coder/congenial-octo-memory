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
            // Configure fall detection parameters
            hu.configLEDLight(hu.eFALLLed, 1);         
            hu.configLEDLight(hu.eHPLed, 1);           
            hu.dmInstallHeight(270);                   
            hu.dmFallTime(5);                          
            hu.dmUnmannedTime(1);                      
            hu.dmFallConfig(hu.eResidenceTime, 200);   
            hu.dmFallConfig(hu.eFallSensitivityC, 3);  
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
    MQTT_connect();
    MQTT_ping();

    // Existing information
    Serial.printf("Existing information:");
    switch (hu.smHumanData(hu.eHumanPresence)) {
        case 0:
            Serial.printf("No one is present\n");
            break;
        case 1:
            Serial.printf("Someone is present\n");
            break;
        default:
            Serial.printf("Read error\n");
    }

    // Motion information
    Serial.printf("Motion information:");
    switch (hu.smHumanData(hu.eHumanMovement)) {
        case 0:
            Serial.printf("None\n");
            break;
        case 1:
            Serial.printf("Still\n");
            break;
        case 2:
            Serial.printf("Active\n");
            break;
        default:
            Serial.printf("Read error\n");
    }

    // Body movement parameters
    Serial.printf("Body movement parameters: %i\n", hu.smHumanData(hu.eHumanMovingRange));
    Serial.printf("Respiration rate: %i\n", hu.getBreatheValue());
    Serial.printf("Heart rate: %i\n", hu.getHeartRate());
    Serial.printf("-----------------------\n");

    // Update NeoPixels
    movementPixel = hu.smHumanData(hu.eHumanMovingRange);
    int pixelNumber = map(movementPixel, 0, 100, 0, PIXELCOUNT - 1);
    pixel.clear();
    PixelFill(0, pixelNumber, color);

    // NFC scan with non-blocking timing
    static unsigned long lastNfcScanTime = 0;
    unsigned long currentTime = millis();
    if (currentTime - lastNfcScanTime >= 100) { // Check NFC every 100ms
        lastNfcScanTime = currentTime;

        if (nfc.scan()) {
            nfcScanned = true; // Set NFC scan flag
            if (nfc.readData(dataRead, READ_BLOCK_NO) == 1) {
                Serial.printf("Block %d read success!\n", READ_BLOCK_NO);
                Serial.printf("Data read (string): %s\n", (char *)dataRead);
                displayNFCData(); // Display scanned NFC data
                myDFPlayer.volume(10); // Set volume
                myDFPlayer.loop(1); // Loop the first mp3
            } else {
                Serial.printf("Block %d read failure!\n", READ_BLOCK_NO);
            }
        } else {
            if (nfcScanned) {
                Serial.printf("Block %d read failure!\n", READ_BLOCK_NO);
            }
        }
    }

    // Update OLED every half second
    static unsigned long lastSecond = 0;
    if ((currentTime - lastSecond) > 500) {
        lastSecond = currentTime;
        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(WHITE);
        display.setCursor(0, 0);
        display.printf(
            "Movement: %i\nRespiratory rate: %i\nHeart Rate: %i\n",
            hu.smHumanData(hu.eHumanMovingRange),
            hu.getBreatheValue(),
            hu.getHeartRate()
        );
        display.display();
    }

    // Publish MQTT data every second
    static unsigned long lastTime = 0;
    if ((currentTime - lastTime > 1000)) {
        mmwave.publish((hu.smHumanData(hu.eHumanMovingRange)));
        lastTime = currentTime;
    }
}

void PixelFill(int startP, int endP, int color) {
    endP = constrain(endP, 0, PIXELCOUNT - 1); // Ensure endP does not exceed PIXELCOUNT - 1

    for (int movementPixel = startP; movementPixel <= endP; movementPixel++) {
        pixel.setPixelColor(movementPixel, rainbow[movementPixel % 7]); // Use rainbow colors
    }
    
    pixel.show(); // Update NeoPixels immediately after setting color
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

   Serial.printf("Connecting to MQTT... ");
   while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
       Serial.printf("Error Code %s\n", mqtt.connectErrorString(ret));
       Serial.printf("Retrying MQTT connection in 5 seconds...\n");
       mqtt.disconnect();
       delay(5000); // Wait 5 seconds and try again
   }
   Serial.printf("MQTT Connected!\n");
}

bool MQTT_ping() {
   static unsigned int last;
   bool pingStatus;

   if ((millis() - last) > 120000) { 
       Serial.printf("Pinging MQTT \n");
       pingStatus = mqtt.ping();
       if (!pingStatus) { 
           Serial.printf("Disconnecting \n");
           mqtt.disconnect();
       }
       last = millis();
   }
   
   return pingStatus;
}
