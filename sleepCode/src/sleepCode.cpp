/* 
 * Project Example of using ULP Mode
 * Author: Brian Rashap
 * Date: 15-APR-2024
 */

#include "Particle.h"
int sleepDuration = 15000;
void sleepULP(); 
SYSTEM_MODE(SEMI_AUTOMATIC);

void setup() {
  Serial.begin(9600);
  waitFor(Serial.isConnected,10000);
  delay(1000);
}

// loop() runs over and over again, as quickly as it can execute.
void loop() {
  Serial.printf("Going to Sleep\n");
  sleepULP();
  delay(5000);
}


void sleepULP() {
  SystemSleepConfiguration config;
  config.mode(SystemSleepMode::ULTRA_LOW_POWER).gpio(D3,RISING).duration(sleepDuration);
  SystemSleepResult result = System.sleep(config);
  delay(3000);
  if (result.wakeupReason() == SystemSleepWakeupReason::BY_GPIO) {
    Serial.printf("Awakened by GPIO %i\n",result.wakeupPin());
  }
  if (result.wakeupReason() == SystemSleepWakeupReason::BY_RTC) {
    Serial.printf("Awakened by RTC\n");
  }
}






