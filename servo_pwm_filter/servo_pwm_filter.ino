/********************************************************
This file is part of the RepRapPNP project, developed at the University of Hamburg.

Author: Daniel Ahlers <2ahlers@informatik.uni-hamburg.de>
Maintainer: Florens Wasserfall <wasserfall@kalanka.de>
License: 'GNU Affero General Public License AGPLv3 http://www.gnu.org/licenses/agpl.html'
*******************************************************/

#include <Servo.h> 
//Servo input pins
int servoInPin1 = 2;
int servoInPin2 = 3;

//Servo output pins
int servoOutPin1 = 9;
int servoOutPin2 = 10;

//Timeout for pwm read in µs
unsigned long timeout = 30000;
//Threshold for new values
unsigned long threshold = 100;

bool servo1Attached = false;
bool servo2Attached = false;

unsigned long currentPWM1;
unsigned long currentPWM2;
unsigned long oldPWM1;
unsigned long oldPWM2;

Servo servo1,servo2;

//Writes servo 1 values and attaches/detaches them
void writeServo1(unsigned int value){
  if (!servo1Attached && value > 0) {
    servo1.attach(servoOutPin1);
    servo1Attached = true;
  }
  if (value > 0) {
    servo1.writeMicroseconds(value);
  }else{
    if(servo1Attached){
      servo1.detach();
      servo1Attached = false;
    }
  }
}

//Writes servo 2 values and attaches/detaches them
void writeServo2(unsigned int value){
  if (!servo2Attached && value > 0){
    servo2.attach(servoOutPin2);
    servo2Attached = true;
  }
  if (value > 0) {
    servo2.writeMicroseconds(value);
  }else{
    if(servo2Attached){
      servo2.detach();
      servo2Attached = false;
    }
  }
}

void setup() {
  Serial.begin(115200);
  
  oldPWM1 = pulseIn(servoInPin1, HIGH,timeout);
  oldPWM2 = pulseIn(servoInPin2, HIGH,timeout);
  
  writeServo1(oldPWM1);
  writeServo2(oldPWM2);
}

void loop() {
  currentPWM1 = pulseIn(servoInPin1, HIGH,timeout);
  currentPWM2 = pulseIn(servoInPin2, HIGH,timeout);

  // If pwm exceeds theshold, the new value is set.
  if ( currentPWM1 >= oldPWM1+threshold || currentPWM1 <= oldPWM1-threshold){
    oldPWM1 = currentPWM1;
    writeServo1(oldPWM1);
  }
  if ( currentPWM2 >= oldPWM2+threshold || currentPWM2 <= oldPWM2-threshold){
    oldPWM2 = currentPWM2;
    writeServo2(oldPWM2);
  }

  //Debug output
  Serial.print("Current1: ");
  Serial.print(currentPWM1);
  Serial.print(" Old1: ");
  Serial.print(oldPWM1);
  Serial.print("   ");
  
  Serial.print("Current2: ");
  Serial.print(currentPWM2);
  Serial.print(" Old2: ");
  Serial.println(oldPWM2);
  
}
