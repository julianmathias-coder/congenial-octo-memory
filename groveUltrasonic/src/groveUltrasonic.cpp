/* 
 * Project myProject
 * Author: Your Name
 * Date: 
 * More accurate than other ultrasonic, but photon needs USB tinker for flashing. Same issue as previous ultrasonic.
 * https://docs.particle.io/firmware/best-practices/firmware-template/
 */


#include "Particle.h"
#include "Grove-Ultrasonic-Ranger.h"

Ultrasonic ultrasonic(A5);
long int RangeInInches;
long int RangeInCentimeters;

SYSTEM_MODE(SEMI_AUTOMATIC);

void setup() {

Serial.begin(9600);
waitFor(Serial.isConnected,10000);

}


void loop() {
  
  RangeInInches = ultrasonic.MeasureInInches();
  Serial.printf("Distance is %li inch\n",RangeInInches);  //0-157 inches
  delay(250);

 RangeInCentimeters = ultrasonic.MeasureInCentimeters(); // two measurements should keep an interval
 Serial.printf("Distance is %li cm\n",RangeInCentimeters);//0~400cm
 delay(250);

}
