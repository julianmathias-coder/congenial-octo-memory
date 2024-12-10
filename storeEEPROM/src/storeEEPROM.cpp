/* 
 * Project myProject
 * Author: Your Name
 * Date: 
 * For comprehensive documentation and examples, please visit:
 * https://docs.particle.io/firmware/best-practices/firmware-template/
 */


#include "Particle.h"

SYSTEM_MODE(SEMI_AUTOMATIC);

// Define EEPROM address for storing state
#define STATE_ADDRESS 0

// Possible states
enum SensorState {
    SLEEP_MODE,
    FALL_MODE
};

// Function to save state to EEPROM
void saveState(SensorState state) {
    EEPROM.put(STATE_ADDRESS, state);
    Serial.printf("State saved: %d\n", state);
}

// Function to load state from EEPROM
SensorState loadState() {
    SensorState state;
    EEPROM.get(STATE_ADDRESS, state);

    // Ensure valid state is loaded
    if (state != SLEEP_MODE && state != FALL_MODE) {
        state = SLEEP_MODE; // Default to SLEEP_MODE if invalid
    }

    Serial.printf("State loaded: %d\n", state);
    return state;
  }

void setup() {
  // Put initialization like pinMode and begin functions here
}

// loop() runs over and over again, as quickly as it can execute.
void loop() {
  // The core of your code will likely live here.

  // Example: Publish event to cloud every 10 seconds. Uncomment the next 3 lines to try it!
  // Log.info("Sending Hello World to the cloud!");
  // Particle.publish("Hello world!");
  // delay( 10 * 1000 ); // milliseconds and blocking - see docs for more info!
}
