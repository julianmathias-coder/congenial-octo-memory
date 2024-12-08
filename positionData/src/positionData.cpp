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
#define BUFFER_SIZE 12

// Function to calculate checksum
uint8_t calculateChecksum(uint8_t *frame, size_t length) {
    uint16_t sum = 0;
    for (size_t i = 0; i < length; i++) {
        sum += frame[i];
    }
    return static_cast<uint8_t>(sum & 0xFF); // Take lower 8 bits
}

SYSTEM_MODE(MANUAL);

void setup() {
    // Initialize Serial for debugging and Serial3 for sensor communication
    Serial.begin(9600);
    waitFor(Serial.isConnected, 10000);
    Serial3.begin(BAUD_RATE);

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

    // Debug: Print sent frame
    Serial.print("Sent: ");
    for (int i = 0; i < sizeof(frame); i++) {
        Serial.printf("%02X ", frame[i]);
    }
    Serial.println();

    // Send command to sensor
    Serial3.write(frame, sizeof(frame));
}

void loop() {
    uint8_t response[BUFFER_SIZE];

    if (Serial3.available()) {
        size_t bytesRead = Serial3.readBytes((char *)response, sizeof(response));

        // Debug: Print received response
        Serial.print("Received: ");
        for (int i = 0; i < bytesRead; i++) {
            Serial.printf("%02X ", response[i]);
        }
        Serial.println();

        if (bytesRead >= BUFFER_SIZE && response[2] == 0x80 && response[3] == 0x05) {
            int16_t x = (response[6] << 8) | response[7];
            int16_t y = (response[8] << 8) | response[9];
            int16_t z = (response[10] << 8) | response[11];

            Serial.printf("Human Position: X=%d cm, Y=%d cm, Z=%d cm\n", x, y, z);
        } else {
            Serial.println("Invalid or unexpected response received.");
        }
    }

    delay(100); // Avoid flooding serial communication
}