/* 
 * Project: finalIoT
 * Author: Julian Mathias
 * Questions: Pointer are used to declare rx/tx pins. Also not sure what hu(&serial1) means vs serial1?
 * Date: 12-01-24
 * For comprehensive documentation and examples, please visit:
 * https://docs.particle.io/firmware/best-practices/firmware-template/
 */

// Include Particle Device OS APIs
#include "Particle.h"
#include "IoTClassroom_CNM.h"
#include "DFRobot_HumanDetection.h"
#include "stdio.h"
#include "application.h"
#include <Adafruit_MQTT.h>
#include "Adafruit_MQTT/Adafruit_MQTT_SPARK.h"
#include "Adafruit_MQTT/Adafruit_MQTT.h"
#include <neopixel.h>
#include "colors.h"
#include "Adafruit_SSD1306.h"
#include <DFRobot_PN532.h>
#include "DFRobotDFPlayerMini.h"
#include "credentials.h"

/************ Global State (you don't need to change this!) ******************/ 
TCPClient TheClient; 

// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details. 
Adafruit_MQTT_SPARK mqtt(&TheClient,AIO_SERVER,AIO_SERVERPORT,AIO_USERNAME,AIO_KEY); 

/****************************** Feeds ***************************************/ 
// Setup Feeds to publish or subscribe 
// Notice MQTT paths for AIO follow the form: <username>/feeds/<feedname>//must include /feeds/ before the feedname!
Adafruit_MQTT_Publish mmwave = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/mmwave");

void MQTT_connect();
bool MQTT_ping();

unsigned int last,lastTime, currentTime, lastSecond;
int pixelNumber;
const int PIXELCOUNT = 16; 
int color;
int movementPixel;

void displayNFCData();
bool nfcScanned = false;

#define PN532_IRQ 2
#define POLLING 0
#define READ_BLOCK_NO 2

DFRobot_PN532_IIC nfc(PN532_IRQ, POLLING);
uint8_t dataRead[16] = {0};

DFRobotDFPlayerMini myDFPlayer;
void printDetail(uint8_t type, int value);
 

Adafruit_NeoPixel pixel(PIXELCOUNT, SPI1, WS2812B);  //SPI1 is D2 pin, PIXELCOUNT is totoal pixels.

void PixelFill (int startP, int endP, int color);  //Where to start and end lighting up the pixels and the color. That's IT! The number of pixels is used in the line above! The end pixel is where we stop filling pixels based on what we want lighted up, not the total number of pixels.

DFRobot_HumanDetection hu(&Serial3);

const int OLED_RESET=-1;
Adafruit_SSD1306 display(OLED_RESET);



SYSTEM_MODE(AUTOMATIC);

void setup() {
  Serial.begin(9600);
  waitFor(Serial.isConnected, 10000);
  Serial1.begin(9200);
  Serial3.begin(115200);
 
  Serial.printf("DFRobot DFPlayer Mini Demo\n");
  Serial.printf("Initializing DFPlayer ... (May take 3~5 seconds)\n");

  myDFPlayer.begin(Serial1);

  while (!nfc.begin()) {
  Serial.printf("NFC initialization failed. Retrying...\n");
  delay(1000);
  }
  Serial.printf("NFC initialized. Waiting for a card...\n");

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C); //0x3c confirmed I2C address in scan
  display.display(); // show splashscreen
  delay(2000);
  display.clearDisplay();   // clears the screen and buffer

  pixel.begin();
  pixel.setBrightness(255);
  pixel.show();  //initialize all pixels to off


  WiFi.on();
  WiFi.connect();
  while (WiFi.connecting()) {
    Serial.printf(".");
 }
    Serial.printf("\n\n");


  Serial.printf("Start initialization\n");
  while (hu.begin() != 0) {
    Serial.printf("init error!!!\n");
    delay(1000);
  }
  Serial.printf("Initialization successful\n");

  Serial.printf("Start switching work mode");
  while (hu.configWorkMode(hu.eSleepMode) != 0) {
    Serial.printf("error!!!\n");
    delay(1000);
  }
  Serial.printf("Work mode switch successful\n");

  Serial.printf("Current work mode:\n");
  switch (hu.getWorkMode()) {
    case 1:
      Serial.printf("Fall detection mode\n");
      break;
    case 2:
      Serial.printf("Sleep detection mode\n");
      break;
    default:
      Serial.printf("Read error\n");
  }

  hu.configLEDLight(hu.eHPLed, 1);  // Set HP LED switch, it will not light up even if the sensor detects a person when set to 0.
  hu.sensorRet();                   // Module reset, must perform sensorRet after setting data, otherwise the sensor may not be usable

  Serial.printf("HP LED status:\n");
  switch (hu.getLEDLightState(hu.eHPLed)) {
    case 0:
      Serial.printf("Off\n");
      break;
    case 1:
      Serial.printf("On\n");
      break;
    default:
      Serial.printf("Read error\n");
  }
 Serial.printf("---------------\n");
}

void loop() {
  MQTT_connect();
  MQTT_ping();
  
  movementPixel=(hu.smHumanData(hu.eHumanMovingRange));  //Movement parameter 0-100
  pixelNumber = map(movementPixel, 0, 100, 0, PIXELCOUNT - 1);  //Since we start at pixel 0, we subtract 1 to get 16 total
  unsigned long currentTime = millis();

  //Update NeoPixels
  pixel.clear();  // Clear all pixels first
  PixelFill(0,movementPixel,color); //Fill pixels based on movement parameter
  
  
   //NFC scan with non-blocking timing
   static unsigned long lastNfcScanTime = 0;
    if (currentTime - lastNfcScanTime >= 100) { // 100ms delay between scans to reduce power consumption
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
        Serial.printf("Block %d read failure!\n", READ_BLOCK_NO); // Print only if read attempt fails
    }
} else {
    if (nfcScanned) { 
        // Only print failure if we attempted a scan
        Serial.printf("Block %d read failure!\n", READ_BLOCK_NO);
    }
}

  //Update OLED every 50ms
  static unsigned long lastSecond = 0; 
    //currentTime=millis(); //put this above already
  if ((currentTime-lastSecond)>50) { 
    lastSecond=currentTime;
// Set OLED text size and color
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  //display.setRotation(3);
  display.printf("Movement: %i\n Respiratory rate: %i\n Heart Rate: %i\n",(hu.smHumanData(hu.eHumanMovingRange)),(hu.getBreatheValue()),(hu.getHeartRate()));
  display.display();
  }

  //Publish MQTT every ten seconds
  static unsigned long lastTime = 0;
  if (currentTime - lastTime > 10000) {  //Need to not overload Adafruit Dashboard
  mmwave.publish((hu.smHumanData(hu.eHumanMovingRange))); //excluding time between pulses to simplify
  lastTime = currentTime; // updated LastTime after publishing to Adafruit
    }
  }

  Serial.printf("Existing information:");
  switch (hu.smHumanData(hu.eHumanPresence)) {
    case 0:
      Serial.printf("No one is present\n");
      break;
    case 1:
      Serial.printf("Someone is present\n");
      //digitalWrite()
      break;
    default:
      Serial.printf("Read error\n");
  }

  Serial.printf("Motion information:");
  switch (hu.smHumanData(hu.eHumanMovement)) {
    case 0:
      Serial.printf("None");
      break;
    case 1:
      Serial.printf("Still");
      break;
    case 2:
      Serial.printf("Active");
      break;
    default:
      Serial.printf("Read error");
  }

  Serial.printf("Body movement parameters: %i\n Respiration rate: %i\n Heart rate: %i\n",(hu.smHumanData(hu.eHumanMovingRange)),(hu.getBreatheValue()),(hu.getHeartRate()));
  Serial.printf("-----------------------");
  //delay(1000);
    

    // uint8_t readBuf[15];
    // uint8_t data = 0x0f;
    // uint16_t x,y;
    // if (hu.getData(0x83, 0x92, 1, &data, readBuf) == 0) {
    //     x = readBuf[6] << 8 | readBuf[7];
    //     y = readBuf[8] << 8 | readBuf[9];
    //     Serial.printf ("X axis: %i\n Y axis: %i\n", x,y);
    // }

//  SystemSleepConfiguration config;
//  config.mode(SystemSleepMode::ULTRA_LOW_POWER).gpio (D6,RISING);
//  SystemSleepResult result  = System.sleep(config);
//  delay(1000);
//  if (result.wakeupReason() == SystemSleepWakeupReason::BY_GPIO) {
//   Serial.printf("Presence Detected  %i\n", result.wakeupPin());
//  }

}

//movementPixel=map((hu.smHumanData(hu.eHumanMovingRange)),0,100,0,10);
//movementPixel=(1/10)*(hu.smHumanData(hu.eHumanMovingRange));
void PixelFill (int startP, int endP, int color) {  //make general for the function
  //Ensure endP does not exceed PIXELCOUNT -1
  endP = constrain(endP,0,PIXELCOUNT -1);
  
  //Light pixels from start and end points
  for (int movementPixel=startP; movementPixel<=endP; movementPixel++) {
  pixel.setPixelColor (movementPixel, rainbow[movementPixel%7]);  //Use rainbow colors Moving through the array of 7 and then starting over at the first color
    }
  pixel.show(); //Update NeoPixels immediately after setting color
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
 
   // Return if already connected.
   if (mqtt.connected()) {
    return;
   }
 
    Serial.print("Connecting to MQTT... ");
 
   while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
        Serial.printf("Error Code %s\n",mqtt.connectErrorString(ret));
        Serial.printf("Retrying MQTT connection in 5 seconds...\n");
        mqtt.disconnect();
        delay(5000);  // wait 5 seconds and try again
   }
   Serial.printf("MQTT Connected!\n");
 }

 bool MQTT_ping() {
   static unsigned int last;
   bool pingStatus;

   if ((millis()-last)>120000) {
       Serial.printf("Pinging MQTT \n");
       pingStatus = mqtt.ping();
       if(!pingStatus) {
         Serial.printf("Disconnecting \n");
         mqtt.disconnect();
       }
       last = millis();
   }
   return pingStatus;
 }

