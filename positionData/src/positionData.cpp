/* 
 * Project myProject
 * Author: Your Name
 * Date: 
 * For comprehensive documentation and examples, please visit:
 * https://docs.particle.io/firmware/best-practices/firmware-template/
 */


#include "Particle.h"

#define BAUD_RATE 115200
#define BUFFER_SIZE 64

uint8_t buffer[BUFFER_SIZE];
size_t bufferIndex = 0;

uint8_t calculateChecksum(uint8_t *frame, size_t length) {
    uint16_t sum = 0;
    for (size_t i = 0; i < length; i++) {
        sum += frame[i];
    }
    return static_cast<uint8_t>(sum & 0xFF);
}

void parseFrame(uint8_t *frame, size_t length) {
    if (length < 9) { // Minimum frame size: Header (2) + Control (1) + Command (1) + Length (2) + Checksum (1) + Tail (2)
        Serial.println("Frame too short");
        return;
    }

    uint16_t dataLength = (frame[4] << 8) | frame[5];
    if (length != dataLength + 9) { // Validate total frame length
        Serial.println("Invalid frame length");
        return;
    }

    uint8_t calculatedChecksum = calculateChecksum(frame + 2, dataLength + 4); // Exclude header and tail
    uint8_t receivedChecksum = frame[length - 3];

    if (calculatedChecksum != receivedChecksum) {
        Serial.printf("Invalid checksum. Calculated: %02X, Received: %02X\n", calculatedChecksum, receivedChecksum);
        return;
    }

    Serial.println("Valid frame received");
    // Process Data based on Control Word and Command Word
}

void setup() {
    Serial.begin(9600);
    waitFor(Serial.isConnected, 10000);
    Serial3.begin(BAUD_RATE);
}

void loop() {
    while (Serial3.available()) {
        uint8_t byte = Serial3.read();

        if (bufferIndex == 0 && byte != 0x53) continue; // Wait for first header byte
        if (bufferIndex == 1 && byte != 0x59) { // Validate second header byte
            bufferIndex = 0;
            continue;
        }

        buffer[bufferIndex++] = byte;

        if (bufferIndex >= BUFFER_SIZE) {
            Serial.println("Buffer overflow. Resetting.");
            bufferIndex = 0;
            continue;
        }

        if (bufferIndex > 7 && buffer[bufferIndex - 2] == 0x54 && buffer[bufferIndex - 1] == 0x43) { // Detect frame tail
            parseFrame(buffer, bufferIndex);
            bufferIndex = 0; // Reset buffer after processing
        }
    }

   //Print raw data from the sensor in hexadecimal format for debugging: 
    Serial.print("Received: ");
for (size_t i = 0; i < bufferIndex; i++) {
    Serial.printf("%02X ", buffer[i]);
}
Serial.println();
} 