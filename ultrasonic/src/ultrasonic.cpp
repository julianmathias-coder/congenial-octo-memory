/* 
 * Project myProject
 * Author: Your Name
 * Date: 
 * Serial monitor won't open unless VUSB unplugged&plugged back in. Photon having issues flashing while US sensor connected.
 * https://docs.particle.io/firmware/best-practices/firmware-template/
 */

// Include Particle Device OS APIs
#include "Particle.h"
#include "HC_SR04.h"


const int TRIG_PIN = A0;
const int ECHO_PIN = A1;
int distance;
int duration;



HC_SR04 rangefinder = HC_SR04(TRIG_PIN, ECHO_PIN);

SYSTEM_MODE(SEMI_AUTOMATIC);



void setup() {
 pinMode(TRIG_PIN, OUTPUT);
 pinMode(ECHO_PIN, INPUT);
 
 Serial.begin (9600);
 waitFor(Serial.isConnected, 10000);

}


void loop() {
   digitalWrite(TRIG_PIN,HIGH); 
   delay(1);
   digitalWrite(TRIG_PIN,LOW);
   duration = pulseIn(ECHO_PIN,HIGH);
   distance = duration / 58.2;  //conversion factor for roundtrip pulse to/from object in air medium (343m/s)
   if(distance > 0 && distance < 200 ){
     Serial.printf("Distance is %icm\n",distance);
     delay(100);
  }
}
