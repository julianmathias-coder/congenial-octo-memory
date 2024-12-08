/* 
 * Project myProject
 * Author: Your Name
 * Date: 
 * For comprehensive documentation and examples, please visit:
 * https://docs.particle.io/firmware/best-practices/firmware-template/
 */

// Include Particle Device OS APIs
#include "Particle.h"
#include <iostream>
#include <vector>
#include <serial/serial.h> // Include a serial communication library like https://github.com/wjwwood/serial

// Function to calculate checksum
uint8_t calculateChecksum(const std::vector<uint8_t>& frame) {
    uint16_t sum = 0;
    for (size_t i = 0; i < frame.size(); ++i) {
        sum += frame[i];
    }
    return static_cast<uint8_t>(sum & 0xFF); // Take lower 8 bits
}

int main() {
    try {
        // Initialize serial connection
        serial::Serial mySerial("/dev/ttyUSB0", 115200, serial::Timeout::simpleTimeout(1000));

        if (!mySerial.isOpen()) {
            std::cerr << "Failed to open serial port!" << std::endl;
            return -1;
        }

        // Construct command frame for Human Position Active Reporting
        std::vector<uint8_t> command = {0x53, 0x59, 0x80, 0x05, 0x00, 0x06};
        uint8_t checksum = calculateChecksum(command);
        command.push_back(checksum);
        command.push_back(0x54);
        command.push_back(0x43);

        // Send command to sensor
        mySerial.write(command);

        std::cout << "Command sent. Waiting for response..." << std::endl;

        // Read response from sensor (example assumes fixed-length response)
        while (true) {
            if (mySerial.available()) {
                std::vector<uint8_t> response(mySerial.available());
                mySerial.read(response.data(), response.size());

                // Parse response (example for human position data)
                if (response.size() >= 12 && response[2] == 0x80 && response[3] == 0x05) {
                    int16_t x = (response[6] << 8) | response[7];
                    int16_t y = (response[8] << 8) | response[9];
                    int16_t z = (response[10] << 8) | response[11];

                    std::cout << "Human Position: X=" << x << " cm, Y=" << y << " cm, Z=" << z << " cm" << std::endl;
                } else {
                    std::cout << "Invalid or unexpected response received." << std::endl;
                }
            }
        }

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    return 0;
}

// Let Device OS manage the connection to the Particle Cloud
SYSTEM_MODE(AUTOMATIC);

// Run the application and system concurrently in separate threads
SYSTEM_THREAD(ENABLED);

// Show system, cloud connectivity, and application logs over USB
// View logs with CLI using 'particle serial monitor --follow'
SerialLogHandler logHandler(LOG_LEVEL_INFO);

// setup() runs once, when the device is first turned on
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
