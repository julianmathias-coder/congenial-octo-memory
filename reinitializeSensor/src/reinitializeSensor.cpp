/* 
 * Project myProject
 * Author: Your Name
 * Date: 
 * For example only
 * https://docs.particle.io/firmware/best-practices/firmware-template/
 */

// Include Particle Device OS APIs
#include "Particle.h"

// Let Device OS manage the connection to the Particle Cloud
SYSTEM_MODE(SEMI_AUTOMATIC);

 void setup() {
    Serial.begin(9600);

    // Load saved state from EEPROM
    SensorState currentState = loadState();

    // Initialize mmWave sensor based on stored state
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
  }


// loop() runs over and over again, as quickly as it can execute.
void loop() {
  // The core of your code will likely live here.

  // Example: Publish event to cloud every 10 seconds. Uncomment the next 3 lines to try it!
  // Log.info("Sending Hello World to the cloud!");
  // Particle.publish("Hello world!");
  // delay( 10 * 1000 ); // milliseconds and blocking - see docs for more info!
}
