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
const int PIXELCOUNT = 30; 
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


  Serial.printf("Start initialization");
  while (hu.begin() != 0) {
    Serial.printf("init error!!!");
    delay(1000);
  }
  Serial.printf("Initialization successful");

  Serial.printf("Start switching work mode");
  while (hu.configWorkMode(hu.eSleepMode) != 0) {
    Serial.printf("error!!!");
    delay(1000);
  }
  Serial.printf("Work mode switch successful");

  Serial.printf("Current work mode:");
  switch (hu.getWorkMode()) {
    case 1:
      Serial.printf("Fall detection mode");
      break;
    case 2:
      Serial.printf("Sleep detection mode");
      break;
    default:
      Serial.printf("Read error");
  }

  hu.configLEDLight(hu.eHPLed, 1);  // Set HP LED switch, it will not light up even if the sensor detects a person when set to 0.
  hu.sensorRet();                   // Module reset, must perform sensorRet after setting data, otherwise the sensor may not be usable

  Serial.printf("HP LED status:");
  switch (hu.getLEDLightState(hu.eHPLed)) {
    case 0:
      Serial.printf("Off");
      break;
    case 1:
      Serial.printf("On");
      break;
    default:
      Serial.printf("Read error");
  }

  Serial.println();
  Serial.println();
}

void loop() {
  MQTT_connect();
  MQTT_ping();


  Serial.printf("Existing information:");
  switch (hu.smHumanData(hu.eHumanPresence)) {
    case 0:
      Serial.printf("No one is present");
      break;
    case 1:
      Serial.printf("Someone is present");
      //digitalWrite()
      break;
    default:
      Serial.println("Read error");
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

  Serial.printf("Body movement parameters:\n",(hu.smHumanData(hu.eHumanMovingRange)));
  Serial.printf("Respiration rate:\n",(hu.getBreatheValue()));
  Serial.printf("Heart rate:\n",(hu.getHeartRate()));
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
     
  movementPixel=(hu.smHumanData(hu.eHumanMovingRange));
     //if (movementPixel !=movementPixel)
     pixel.clear();
     PixelFill(0,movementPixel,color);
    
   static unsigned long lastNfcScanTime = 0;
   unsigned long currentTime = millis();

   if (currentTime - lastNfcScanTime >= 100) { // 100ms delay between scans to reduce power consumption
    lastNfcScanTime = currentTime;
   
   if (nfc.scan()) {
     nfcScanned = true; // Set NFC scan flag
    // (hu.begin());
    // (hu.configWorkMode(hu.eFallingMode));
     (nfc.readData(dataRead, READ_BLOCK_NO) == 1);
     Serial.printf("Block %d read success!\n", READ_BLOCK_NO);
     Serial.printf("Data read (string): %s\n", (char *)dataRead);
     displayNFCData();  //This should clear and display scanned NFC characters
     delay (3000); //testing to see if NFC characteres displayed

    myDFPlayer.volume(10); //Set volume to 30 if NFC not scanned
    myDFPlayer.loop(1); //Loop the first mp3
   } 
   else {
     Serial.printf("Block %d read failure!\n", READ_BLOCK_NO);
     }
   }
   

    currentTime=millis();
  if ((currentTime-lastSecond)>500) { //half second
    lastSecond=millis ();
// Set OLED text size and color
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  //display.setRotation(3);
  display.printf("Movement: %i\n", (hu.smHumanData(hu.eHumanMovingRange)));
  display.display();
  }

if((millis() - lastTime > 1000)) {  //Need to not overload Adafruit Dashboard
    mmwave.publish((hu.smHumanData(hu.eHumanMovingRange))); //excluding time between pulses to simplify
   // Serial.printf("Published movement: %0.1f RPM\n", RPMCalc);
}
    lastTime = millis(); // updated LastTime after publishing to Adafruit
  }

//movementPixel=map((hu.smHumanData(hu.eHumanMovingRange)),0,100,0,10);
//movementPixel=(1/10)*(hu.smHumanData(hu.eHumanMovingRange));
void PixelFill (int startP, int endP, int color) {  //make general for the function
  //for (int pixelNumber=startP; pixelNumber<=endP; pixelNumber++) { 
  for (int movementPixel=startP; movementPixel<=endP; movementPixel++) {
  //pixel.setPixelColor (pixelNumber, rainbow[pixelNumber%7]); //Moving through the array of 7 and then starting over at the first color
  pixel.setPixelColor (movementPixel, rainbow[movementPixel%7]);
}
  //pixel.clear();
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

