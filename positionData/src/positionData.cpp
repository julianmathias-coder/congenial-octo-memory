/* 
 * Project myProject
 * Author: Your Name
 * Date: 
 * For comprehensive documentation and examples, please visit:
 * https://docs.particle.io/firmware/best-practices/firmware-template/
 */

#include "Particle.h"

// Define serial communication parameters
#define BAUD_RATE 115200

// Function to calculate checksum
uint8_t calculateChecksum(uint8_t *frame, size_t length) {
    uint16_t sum = 0;
    for (size_t i = 0; i < length; i++) {
        sum += frame[i];
    }
    return static_cast<uint8_t>(sum & 0xFF); // Take lower 8 bits
}

void setup() {
    // Initialize Serial for debugging and Serial1 for sensor communication
    Serial.begin(9600); // USB Debugging
    Serial3.begin(BAUD_RATE); // UART Communication with Sensor

    // Wait for serial connection
    while (!Serial) {
        delay(100);
    }

    // Construct command frame for enabling Human Position Active Reporting
    uint8_t command[] = {0x53, 0x59, 0x80, 0x05, 0x00, 0x06};
    uint8_t checksum = calculateChecksum(command, sizeof(command));
    
    // Append checksum and frame tail
    uint8_t frame[] = {command[0], command[1], command[2], command[3], 
                       command[4], command[5], checksum, 0x54, 0x43};

    // Send command to sensor
    Serial1.write(frame, sizeof(frame));
    Serial.println("Command sent to enable Human Position Active Reporting.");
}

void loop() {
    // Check if data is available from the sensor
    if (Serial3.available()) {
        uint8_t response[12]; // Example response length for position data
        size_t bytesRead = Serial3.readBytes(response, sizeof(response));

        if (bytesRead >= 12 && response[2] == 0x80 && response[3] == 0x05) {
            // Parse x, y, z coordinates from response data
            int16_t x = (response[6] << 8) | response[7];
            int16_t y = (response[8] << 8) | response[9];
            int16_t z = (response[10] << 8) | response[11];

            // Print human position data to serial monitor
            Serial.printf("Human Position: X=%d cm, Y=%d cm, Z=%d cm\n", x, y, z);
        } else {
            Serial.println("Invalid or unexpected response received.");
        }
    }

    delay(100); // Add a small delay to avoid flooding the serial communication
}