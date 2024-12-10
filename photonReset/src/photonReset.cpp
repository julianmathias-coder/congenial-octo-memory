/* 
 * Project myProject
 * Author: Your Name
 * Date: 
 * Not setup, example only
 * https://docs.particle.io/firmware/best-practices/firmware-template/
 */

// Include Particle Device OS APIs
#include "Particle.h"

// Let Device OS manage the connection to the Particle Cloud
SYSTEM_MODE(SEMI_AUTOMATIC);


// setup() runs once, when the device is first turned on
void setup() {
  // Put initialization like pinMode and begin functions here
}

// loop() runs over and over again, as quickly as it can execute.
void loop() {
  if (nfc.scan()) {
    if (nfc.readData(dataRead, READ_BLOCK_NO) == 1) {
        Serial.printf("NFC Data: %s\n", (char *)dataRead);

        // Determine mode based on NFC data
        if (strcmp((char *)dataRead, "SLEEP") == 0) {
            saveState(SLEEP_MODE);
        } else if (strcmp((char *)dataRead, "FALL") == 0) {
            saveState(FALL_MODE);
        }

        // Reset device to apply changes
        System.reset();
    }
}
}
