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
#define BUFFER_SIZE 64 // Adjust based on maximum frame size

// Function to calculate checksum
uint8_t calculateChecksum(uint8_t *frame, size_t length) {
    uint16_t sum = 0;
    for (size_t i = 0; i < length; i++) {
        sum += frame[i];
    }
    return static_cast<uint8_t>(sum & 0xFF); // Take lower 8 bits
}

// Function to parse a valid frame
void parseFrame(uint8_t *frame, size_t length) {
    if (length < 7) { // Minimum frame size: Header (2) + Control (1) + Command (1) + Length (2) + Checksum (1)
        Serial.println("Frame too short");
        return;
    }

    // Validate checksum
    uint8_t calculatedChecksum = calculateChecksum(frame + 2, length - 5); // Exclude header and tail
    uint8_t receivedChecksum = frame[length - 3]; // Checksum is before tail

    if (calculatedChecksum != receivedChecksum) {
        Serial.printf("Invalid checksum. Calculated: %02X, Received: %02X\n", calculatedChecksum, receivedChecksum);
        return;
    }

    // Parse control word and command word
    uint8_t controlWord = frame[2];
    uint8_t commandWord = frame[3];

    if (controlWord == 0x80 && commandWord == 0x05) { // Example: Human Position Active Reporting
        int16_t x = (frame[6] << 8) | frame[7];
        int16_t y = (frame[8] << 8) | frame[9];
        int16_t z = (frame[10] << 8) | frame[11];

        Serial.printf("Human Position: X=%d cm, Y=%d cm, Z=%d cm\n", x, y, z);
    } else {
        Serial.println("Unknown control/command word");
    }
}

SYSTEM_MODE(MANUAL);


void setup() {
    Serial.begin(9600);
    waitFor(Serial.isConnected, 10000);
    Serial3.begin(BAUD_RATE);

    Serial.println("C1001 mmWave Sensor Initialized");
}

void loop() {
    static uint8_t buffer[BUFFER_SIZE];
    static size_t bufferIndex = 0;

    while (Serial3.available()) {
        uint8_t byte = Serial3.read();

        // Add byte to buffer
        buffer[bufferIndex++] = byte;

        // Check for valid frame header
        if (bufferIndex >= 2 && buffer[0] == 0x53 && buffer[1] == 0x59) {
            // Check if we have enough bytes for a complete frame
            if (bufferIndex >= BUFFER_SIZE || 
                (bufferIndex >= 2 && buffer[BUFFER_SIZE - 2] == 0x54 && buffer[BUFFER_SIZE - 1] == 0x43)) {


    delay(100); // Avoid flooding serial communication
  //Print raw data from the sensor in hexadecimal format for debugging: 
    Serial.print("Received: ");
for (size_t i = 0; i < bufferIndex; i++) {
    Serial.printf("%02X ", buffer[i]);
}
Serial.println();


                // Parse the frame
                parseFrame(buffer, bufferIndex);

                // Reset the buffer index for the next frame
                bufferIndex = 0;
            }
        }

        // Prevent overflow in case of invalid data stream
        if (bufferIndex >= BUFFER_SIZE) {
            Serial.println("Buffer overflow. Resetting.");
            bufferIndex = 0;
        }

    }
}