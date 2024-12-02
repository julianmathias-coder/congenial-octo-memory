/* 
 * Project: finalIoT
 * Author: Julian Mathias
 * Date: 12-01-24
 * For comprehensive documentation and examples, please visit:
 * https://docs.particle.io/firmware/best-practices/firmware-template/
 */

// Include Particle Device OS APIs
#include "Particle.h"
#include "DFRobot_HumanDetection.h"
#include "stdio.h"
#include "application.h"

DFRobot_HumanDetection hu(&Serial1);
SYSTEM_MODE(SEMI_AUTOMATIC);

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

  Serial.printf("Start initialization/n");
  while (hu.begin() != 0) {
    Serial.printf("init error!!!/n");
    delay(1000);
  }
  Serial.printf("Initialization successful/n");

  Serial.printf("Start switching work mode/n");
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

  Serial.printf("HP LED status: \n");
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

  Serial.println();
  Serial.println();
}

void loop() {
  Serial.printf("Existing information: \n");
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

  Serial.printf("Motion information: \n");
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

  Serial.print("Body movement parameters: \n");
  Serial.println(hu.smHumanData(hu.eHumanMovingRange));
  Serial.print("Respiration rate: ");
  Serial.println(hu.getBreatheValue());
  Serial.print("Heart rate: ");
  Serial.println(hu.getHeartRate());
  Serial.println("-----------------------");
  delay(1000);
}