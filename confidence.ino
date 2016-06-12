// ---------------------------------------------------------------------------
// Confidenc Prototype
// By: Jordan Shaw
// Libs: NewPing, Adafruit MotorShield, AccelStepper, PWMServoDriver
// ---------------------------------------------------------------------------

#define DEBUG false

#include <NewPing.h>
#include <Array.h>

#include <AccelStepper.h>
#include <Wire.h>
#include <Adafruit_MotorShield.h>
#include "utility/Adafruit_MS_PWMServoDriver.h"

#define TRIGGER_PIN  12  // Arduino pin tied to trigger pin on the ultrasonic sensor.
#define ECHO_PIN     11  // Arduino pin tied to echo pin on the ultrasonic sensor.
#define MAX_DISTANCE 400 // Maximum distance we want to ping for (in centimeters). Maximum sensor distance is rated at 400-500cm.

// Controls Installation Mode
// 0 = scanning
// 1 = go to furthest distance
int mode = 2;
int foundIndex = 0;
int distanceThreshold = 80;

// Scann interval
//const long interval = 2500;
const long interval = 1500;
unsigned long previousMillis = 0;

// Wait time between scans
const long waitInterval = 10000;
unsigned long waitPreviousMillis = 0;

int currentDistance = 0;
int previousDistance = 0;

int motorStep = 2;
int motorDirection = 0;
int motorStartPosition = 0;
int motorCurrentPosition = 0;

//int motorMaxPosition = 26;

// Half the rotation for testing
int motorMaxPosition = 14;

NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE); // NewPing setup of pins and maximum distance.

// Create the motor shield object with the default I2C address
Adafruit_MotorShield AFMS = Adafruit_MotorShield(); 

// Connect a stepper motor with 50 steps per revolution (7.2 degree)
// to motor port #2 (M3 and M4)
Adafruit_StepperMotor *myMotor = AFMS.getStepper(50, 2);
//AccelStepper stepper1(forwardstep1, backwardstep1);

// Setting up the different values for the sonar values while rotating
const byte size = 14;
int rawArray[size] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13};
int sensorArrayValue[size];
Array<int> array = Array<int>(sensorArrayValue, size);

void setup() {
  // Open serial monitor at 115200 baud to see ping results.
  Serial.begin(115200);

  // create with the default frequency 1.6KHz
  AFMS.begin();
  // default 10 rpm   
  myMotor->setSpeed(50);
}

void loop() {
  
  unsigned long currentMillis = millis();
  
  if(currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    // if mode is go to furthest distance go to that place / step
    if (mode == 1){
      int maxDis = constrain(array.getMax(), 5, 400);

      Serial.print("$$$$$$ array.getMax(): ");
      Serial.println(array.getMax());
      
      int maxIndex = constrain(array.getMaxIndex(), 0, (int)size);

      Serial.print("array.size(): ");
      Serial.println(array.size());

      Serial.print("getMax: ");
      Serial.print(maxDis); // Send ping, get distance in cm and print result (0 = outside set distance range)
      Serial.println("cm");

      Serial.print("getMaxIndex: ");
      Serial.println(maxIndex); // Send ping, get distance in cm and print result (0 = outside set distance range)

      if (foundIndex == 0){

        int stepToIndex = maxIndex * 2;
        int stepsToGetToPosition = motorMaxPosition - stepToIndex;
  
        myMotor->step(stepsToGetToPosition, BACKWARD, MICROSTEP); 
        motorCurrentPosition = abs(stepToIndex);

        foundIndex = 1;
        waitPreviousMillis = millis();
      }

      // While the object is paused, if the max distance in that step's index changes more than 10/20 cm 
      // reset the mode and foundIndex and rescan.
      // The change in distance means there was a change in position of the observers

      currentDistance = sonar.ping_cm();

      if (currentDistance <= 5){
        return;
      }

      Serial.print("**** currentDistance: ");
      Serial.println(currentDistance);

      Serial.print("**** maxDis: ");
      Serial.println(maxDis);

      Serial.print("**** distanceThreshold: ");
      Serial.println(distanceThreshold);

      Serial.print("**** currentDistance - maxDis >= distanceThreshold: ");
      Serial.println(abs(currentDistance - maxDis));
      
      if(abs(currentDistance - maxDis) >= distanceThreshold){
        // Break out of the wait

        if(abs(currentDistance - maxDis) < 5 || distanceThreshold == 400){
          return;
        }
        foundIndex = 0;
        mode = 0;

        Serial.println("**** WE EVER GET HERE?");
        return;
      }

      unsigned long waitCurrentMillis = millis();
      if(waitCurrentMillis - waitPreviousMillis >= waitInterval) {
        waitPreviousMillis = waitCurrentMillis;
        Serial.println("GET HERE?!");
        foundIndex = 0;
        mode = 0;

        Serial.print("MODE: ");
        Serial.println(mode);
        Serial.print("foundIndex: ");
        Serial.println(foundIndex);
      }

      return;
    }
    
    if (DEBUG == true){
      
      // Debug / Prototype Motor
      currentDistance = random(5, 400); 

      Serial.print("Ping: ");
      Serial.print(currentDistance); // Send ping, get distance in cm and print result (0 = outside set distance range)
      Serial.println("cm");

      int currentStep = abs(motorCurrentPosition) / 2;
      sensorArrayValue[currentStep] = currentDistance;
      
    } else {
      
      // Gets the sonar distance
      currentDistance = sonar.ping_cm();
      
      if(currentDistance != 0){
        Serial.print("Ping: ");
        Serial.print(currentDistance); // Send ping, get distance in cm and print result (0 = outside set distance range)
        Serial.println("cm");
  
        int currentStep = abs(motorCurrentPosition) / 2;
        sensorArrayValue[currentStep] = currentDistance;
      }
      
    }

    //  Controls the motor
    if(motorCurrentPosition <= motorMaxPosition && motorDirection == 0){
      myMotor->step(motorStep, FORWARD, MICROSTEP); 
      motorCurrentPosition += motorStep;
    } else if (motorCurrentPosition >= motorStartPosition && motorDirection == 1) {
      myMotor->step(motorStep, BACKWARD, MICROSTEP); 
      motorCurrentPosition -= motorStep;
    }

    Serial.print("motorCurrentPosition: ");
    Serial.println(motorCurrentPosition);

    Serial.print("motorDirection: ");
    Serial.println(motorDirection);

    // Updates the direction of the motor
    if (motorCurrentPosition <= 0){
      // forwards
      motorDirection = 0;
    } else if (motorCurrentPosition >= (motorMaxPosition -1)) {
      // backwards
      motorDirection = 1;
      mode = 1;
    }

    previousDistance = currentDistance;
    
  }

}

// you can change these to DOUBLE or INTERLEAVE or MICROSTEP!
// wrappers for the first motor!
void forwardstep1() {  
  myMotor->onestep(FORWARD, MICROSTEP);
}

void backwardstep1() {  
  myMotor->onestep(BACKWARD, MICROSTEP);
}


