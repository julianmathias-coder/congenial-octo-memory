/* 
 * Project myProject
 * Author: Your Name
 * Date: 
 * For comprehensive documentation and examples, please visit:
 * https://docs.particle.io/firmware/best-practices/firmware-template/
 */

// Include Particle Device OS APIs
#include "Particle.h"
#include "DFRobot_HumanDetection.h"

DFRobot_HumanDetection hu(&Serial3);

// Let Device OS manage the connection to the Particle Cloud
SYSTEM_MODE(SEMI_AUTOMATIC);

// Run the application and system concurrently in separate threads
SYSTEM_THREAD(ENABLED);

// Show system, cloud connectivity, and application logs over USB
// View logs with CLI using 'particle serial monitor --follow'
SerialLogHandler logHandler(LOG_LEVEL_INFO);

// setup() runs once, when the device is first turned on
void setup() {
  Serial.begin(9600);
  waitFor(Serial.isConnected, 10000);
  Serial3.begin(115200);

    Serial.println("Start initialization");
  while (hu.begin() != 0) {
    Serial.println("init error!!!");
    delay(1000);
  }
  Serial.println("Initialization successful");

  Serial.println("Start switching work mode");
  while (hu.configWorkMode(hu.eFallingMode) != 0) {
    Serial.println("error!!!");
    delay(1000);
  }
  Serial.println("Work mode switch successful");

//not sure if this is needed
  hu.configLEDLight(hu.eFALLLed, 1);         // Set HP LED switch, it will not light up even if the sensor detects a person present when set to 0.
  hu.configLEDLight(hu.eHPLed, 1);           // Set FALL LED switch, it will not light up even if the sensor detects a person falling when set to 0.
  hu.dmInstallHeight(270);                   // Set installation height, it needs to be set according to the actual height of the surface from the sensor, unit: CM.
  hu.dmFallTime(5);                          // Set fall time, the sensor needs to delay the current set time after detecting a person falling before outputting the detected fall, this can avoid false triggering, unit: seconds.
  hu.dmUnmannedTime(1);                      // Set unattended time, when a person leaves the sensor detection range, the sensor delays a period of time before outputting a no person status, unit: seconds.
  hu.dmFallConfig(hu.eResidenceTime, 200);   // Set dwell time, when a person remains still within the sensor detection range for more than the set time, the sensor outputs a stationary dwell status. Unit: seconds.
  hu.dmFallConfig(hu.eFallSensitivityC, 3);  // Set fall sensitivity, range 0~3, the larger the value, the more sensitive.
  hu.sensorRet();                            // Module reset, must perform sensorRet after setting data, otherwise the sensor may not be usable.




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

  //hu.configLEDLight(hu.eHPLed, 1);  // Set HP LED switch, it will not light up even if the sensor detects a person when set to 0.
  //hu.sensorRet();                   // Module reset, must perform sensorRet after setting data, otherwise the sensor may not be usable

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

  Serial.print("FALL status:");
  switch (hu.getLEDLightState(hu.eFALLLed)) {
    case 0:
      Serial.println("Off");
      break;
    case 1:
      Serial.println("On");
      break;
    default:
      Serial.println("Read error");
  }

  Serial.print("Radar installation height: ");
  Serial.print(hu.dmGetInstallHeight());
  Serial.println(" cm");
  Serial.print("Fall duration: ");
  Serial.print(hu.getFallTime());
  Serial.println(" seconds");
  Serial.print("Unattended duration: ");
  Serial.print(hu.getUnmannedTime());
  Serial.println(" seconds");
  Serial.print("Dwell duration: ");
  Serial.print(hu.getStaticResidencyTime());
  Serial.println(" seconds");
  Serial.print("Fall sensitivity: ");
  Serial.print(hu.getFallData(hu.eFallSensitivity));
  Serial.println(" seconds");
  Serial.println("===============================");
}

// loop() runs over and over again, as quickly as it can execute.
void loop() {
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
  // Serial.print("Respiration rate: ");
  // Serial.println(hu.getBreatheValue());
  // Serial.print("Heart rate: ");
  // Serial.println(hu.getHeartRate());
  // Serial.println("-----------------------");
  //delay(1000);
Serial.print("Fall status:");
  switch (hu.getFallData(hu.eFallState)) {
    case 0:
      Serial.println("Not fallen");
      break;
    case 1:
      Serial.println("Fallen");
      break;
    default:
      Serial.println("Read error");
  }
  
  Serial.print("Stationary dwell status: ");
  switch (hu.getFallData(hu.estaticResidencyState)) {
    case 0:
      Serial.println("No stationary dwell");
      break;
    case 1:
      Serial.println("Stationary dwell present");
      break;
    default:
      Serial.println("Read error");
  }
  Serial.println();
  delay(1000);
}

