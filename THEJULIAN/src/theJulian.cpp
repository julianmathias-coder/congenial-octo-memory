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
  Serial.print("Existing information:");
  switch (hu.smHumanData(hu.eHumanPresence)) {
    case 0:
      Serial.println("No one is present");
      break;
    case 1:
      Serial.println("Someone is present");
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
}