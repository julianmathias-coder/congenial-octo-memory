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
#include "DFRobot_HumanDetection.h"
#include "stdio.h"
#include "application.h"
#include <Adafruit_MQTT.h>
#include "Adafruit_MQTT/Adafruit_MQTT_SPARK.h"
#include "Adafruit_MQTT/Adafruit_MQTT.h"
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

unsigned int last,lastTime;

DFRobot_HumanDetection hu(&Serial1);



SYSTEM_MODE(AUTOMATIC);

void setup() {
  Serial.begin(9600);
  waitFor(Serial.isConnected, 10000);
  Serial1.begin(115200);

   WiFi.on();
   WiFi.connect();
   while (WiFi.connecting()) {
    Serial.printf(".");
 }
    Serial.printf("\n\n");

  Serial1.println("Start initialization");
  while (hu.begin() != 0) {
    Serial1.println("init error!!!");
    delay(1000);
  }
  Serial.println("Initialization successful");

  Serial.println("Start switching work mode");
  while (hu.configWorkMode(hu.eSleepMode) != 0) {
    Serial.println("error!!!");
    delay(1000);
  }
  Serial.println("Work mode switch successful");

  Serial.print("Current work mode:");
  switch (hu.getWorkMode()) {
    case 1:
      Serial.println("Fall detection mode");
      break;
    case 2:
      Serial.println("Sleep detection mode");
      break;
    default:
      Serial.println("Read error");
  }

  hu.configLEDLight(hu.eHPLed, 1);  // Set HP LED switch, it will not light up even if the sensor detects a person when set to 0.
  hu.sensorRet();                   // Module reset, must perform sensorRet after setting data, otherwise the sensor may not be usable

  Serial.print("HP LED status:");
  switch (hu.getLEDLightState(hu.eHPLed)) {
    case 0:
      Serial.println("Off");
      break;
    case 1:
      Serial.println("On");
      break;
    default:
      Serial.println("Read error");
  }

  Serial.println();
  Serial.println();
}

void loop() {
 MQTT_connect();
 MQTT_ping();

  Serial.print("Existing information:");
  switch (hu.smHumanData(hu.eHumanPresence)) {
    case 0:
      Serial.println("No one is present");
      break;
    case 1:
      Serial.println("Someone is present");
      //digitalWrite()
      break;
    default:
      Serial.println("Read error");
  }

  Serial.print("Motion information:");
  switch (hu.smHumanData(hu.eHumanMovement)) {
    case 0:
      Serial.println("None");
      break;
    case 1:
      Serial.println("Still");
      break;
    case 2:
      Serial.println("Active");
      break;
    default:
      Serial.println("Read error");
  }

  Serial.print("Body movement parameters: ");
  Serial.println(hu.smHumanData(hu.eHumanMovingRange));
  Serial.print("Respiration rate: ");
  Serial.println(hu.getBreatheValue());
  Serial.print("Heart rate: ");
  Serial.println(hu.getHeartRate());
  Serial.println("-----------------------");
  delay(1000);

if((millis() - lastTime > 1000)) {  //Need to not overload Adafruit Dashboard
    mmwave.publish((hu.smHumanData(hu.eHumanMovingRange))); //excluding time between pulses to simplify
   // Serial.printf("Published movement: %0.1f RPM\n", RPMCalc);
}
    lastTime = millis(); // updated LastTime after publishing to Adafruit
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
