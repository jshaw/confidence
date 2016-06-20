#include <Arduino.h>

// ---------------------------------------------------------------------------
// Confidenc Prototype
// By: Jordan Shaw
// Libs: NewPing, Adafruit MotorShield, AccelStepper, PWMServoDriver
// ---------------------------------------------------------------------------

// Commands

// Go / Start
// g = 103 

// Stop
// s = 115

// Next
// n = 110

// Previous
// p = 112

// Set config
// c = 99

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
int distanceThreshold = 40;
int maxError = 5;
bool paused = false;
bool breakout = false;

// defaults to stop
int incomingByte = 115;

// Scann interval
//const long interval = 2500;
const long interval = 1500;
unsigned long previousMillis = 0;

// Wait time between scans
const long waitInterval = 10000;
unsigned long waitPreviousMillis = 0;

int currentDistance = 0;
int previousDistance = 0;

int motorStep = 1;
int motorDirection = 0;
int motorStartPosition = 0;
int motorCurrentPosition = 0;

//int motorMaxPosition = 26;

// Half the rotation for testing
int motorMaxPosition = 20;

bool moveMotor = false;

NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE); // NewPing setup of pins and maximum distance.

// How frequently are we going to send out a ping (in milliseconds). 50ms would be 20 times a second.
unsigned int pingSpeed = 40;
// Holds the next ping time.
unsigned long pingTimer;

unsigned long currentMillis = millis();

// Create the motor shield object with the default I2C address
Adafruit_MotorShield AFMS = Adafruit_MotorShield(); 

// Connect a stepper motor with 50 steps per revolution (7.2 degree)
// to motor port #2 (M3 and M4)
Adafruit_StepperMotor *myMotor = AFMS.getStepper(50, 2);
//AccelStepper stepper1(forwardstep1, backwardstep1);

// Setting up the different values for the sonar values while rotating
const byte size = 26;
// int rawArray[size] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25};
// int sensorArrayValue[size] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25};

int sensorArrayValue[size] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19};

// int sensorArrayValue[size];
Array<int> array = Array<int>(sensorArrayValue, size);

void setup() {
  // Open serial monitor at 115200 baud to see ping results.
  Serial.begin(115200);

  // Sets the ping timer
  pingTimer = millis();

  // create with the default frequency 1.6KHz
  AFMS.begin();
  // default 10 rpm   
  myMotor->setSpeed(50);
}

void loop() {

  if (Serial.available() > 0) {
    // read the incoming byte:
    incomingByte = Serial.read();

    // say what you got:
    Serial.print("I received: ");
    Serial.println(incomingByte, DEC);
    
  }

    
  // g = 103
  // s = 115
  // n = 110
  // p = 112
  // c = 99 

  // go
  if (incomingByte == 103) {
    
  // next
  } else if(incomingByte == 110){
    myMotor->step(motorStep, FORWARD, MICROSTEP); 
    motorCurrentPosition += motorStep;
    // sets it to stop
    incomingByte = 115;

  // previous
  } else if(incomingByte == 112){
    myMotor->step(motorStep, BACKWARD, MICROSTEP); 
    motorCurrentPosition -= motorStep;
//    // sets it to stop
    incomingByte = 115;

  // stop
  } else if(incomingByte == 115){
    // Serial.println("IN STOPs");

  // Config... sets the current position to what index
  } else if(incomingByte == 99){
    // Serial.println("In Config");
    motorCurrentPosition = motorMaxPosition / 2;
    Serial.println(motorCurrentPosition);
  }

  // stop the loop if it is anything other than 'g' (go)
  if (incomingByte != 103){
    return;
  }

   currentMillis = millis();

  if (currentMillis >= pingTimer) {   // pingSpeed milliseconds since last ping, do another ping.
    pingTimer += pingSpeed;      // Set the next ping time.
    sonar.ping_timer(echoCheck); // Send out the ping, calls "echoCheck" function every 24uS where you can check the ping status.
    // Serial.print("*****");
    // Serial.println(currentDistance);

    moveMotor = true;
    
  } else {
    moveMotor = false;
  }

  // Do other stuff here, really. Think of it as multi-tasking.
  
  // if(currentMillis - previousMillis >= interval) {
  //   previousMillis = currentMillis;

  //   // if mode is go to furthest distance go to that place / step
  //   if (mode == 1){
  //     int maxDis = constrain(array.getMax(), 5, 400);

  //     Serial.print("$$$$$$ array.getMax(): ");
  //     Serial.println(array.getMax());
      
  //     int maxIndex = constrain(array.getMaxIndex(), 0, (int)size);

  //     Serial.print("array.size(): ");
  //     Serial.println(array.size());

  //     Serial.print("getMax: ");
  //     Serial.print(maxDis); // Send ping, get distance in cm and print result (0 = outside set distance range)
  //     Serial.println("cm");

  //     Serial.print("getMaxIndex: ");
  //     Serial.println(maxIndex); // Send ping, get distance in cm and print result (0 = outside set distance range)

  //     if (foundIndex == 0){

  //       int stepToIndex = maxIndex * 2;
  //       int stepsToGetToPosition = motorMaxPosition - stepToIndex;
  
  //       myMotor->step(stepsToGetToPosition, BACKWARD, MICROSTEP); 
  //       motorCurrentPosition = abs(stepToIndex);

  //       foundIndex = 1;
  //       waitPreviousMillis = millis();
  //     }

  //     // While the object is paused, if the max distance in that step's index changes more than 10/20 cm 
  //     // reset the mode and foundIndex and rescan.
  //     // The change in distance means there was a change in position of the observers

  //     currentDistance = sonar.ping_cm();

  //     if (currentDistance <= 5){
  //       return;
  //     }

  //     Serial.print("**** currentDistance: ");
  //     Serial.println(currentDistance);

  //     Serial.print("**** maxDis: ");
  //     Serial.println(maxDis);

  //     Serial.print("**** distanceThreshold: ");
  //     Serial.println(distanceThreshold);

  //     Serial.print("**** currentDistance - maxDis >= distanceThreshold: ");
  //     Serial.println(abs(currentDistance - maxDis));
      
  //     if(abs(currentDistance - maxDis) >= distanceThreshold){
  //       // Break out of the wait

  //       if(abs(currentDistance - maxDis) < 5 || distanceThreshold == 400){
  //         return;
  //       }
  //       foundIndex = 0;
  //       mode = 0;

  //       Serial.println("**** WE EVER GET HERE?");
  //       return;
  //     }

  //     unsigned long waitCurrentMillis = millis();
  //     if(waitCurrentMillis - waitPreviousMillis >= waitInterval) {
  //       waitPreviousMillis = waitCurrentMillis;
  //       Serial.println("GET HERE?!");
  //       foundIndex = 0;
  //       mode = 0;

  //       Serial.print("MODE: ");
  //       Serial.println(mode);
  //       Serial.print("foundIndex: ");
  //       Serial.println(foundIndex);
  //     }

  //     return;
  //   }
    
  // OLD DEBUG
  // ==============
  //   if (DEBUG == true){
      
  //     // Debug / Prototype Motor
  //     currentDistance = random(5, 400); 

  //     Serial.print("Ping: ");
  //     Serial.print(currentDistance); // Send ping, get distance in cm and print result (0 = outside set distance range)
  //     Serial.println("cm");

  //     int currentStep = abs(motorCurrentPosition) / 2;
  //     sensorArrayValue[currentStep] = currentDistance;
      
  //   } else {
      
      // Gets the sonar distance
      // currentDistance = sonar.ping_cm();
      
      // if(currentDistance != 0){
      //   Serial.print("Ping: ");
      //   Serial.print(currentDistance); // Send ping, get distance in cm and print result (0 = outside set distance range)
      //   Serial.println("cm");
  
      //   int currentStep = abs(motorCurrentPosition) / 2;
      //   sensorArrayValue[currentStep] = currentDistance;
      // }
      
  //   }
  // END OLD DEBUG
  // ==============




  // NOTICE
  // ==============
  // BELOW HERE IS THE WIP




    if(moveMotor == true){

      if (mode == 1){
        int maxDis = constrain(array.getMax(), 5, 400);
        int maxIndex = constrain(array.getMaxIndex(), 0, (int)size);

        // Serial.println("=======");
        // Serial.println(array.size());
        // Serial.println("=======");
//
        if (foundIndex == 0){
          // Serial.println("**** WE EVER GET HERE?");
          int stepToIndex = maxIndex;
          int stepsToGetToPosition = motorMaxPosition - stepToIndex;

          // MICROSTEP
          // DOUBLE
          // INTERLEAVE
          myMotor->step(stepsToGetToPosition, BACKWARD, MICROSTEP); 
          motorCurrentPosition = abs(stepToIndex);

          foundIndex = 1;
          // waitPreviousMillis = millis();
        }
//
//        // While the object is paused, if the max distance in that step's index changes more than 10/20 cm 
//        // reset the mode and foundIndex and rescan.
//        // The change in distance means there was a change in position of the observers
//
//        // currentDistance = sonar.ping_cm();
//
//        if (currentDistance <= 5){
//          return;
//        }
//



// good logging here
// right below


//           Serial.print("**** maxIndex: ");
//           Serial.println(maxIndex);

//           Serial.print("**** currentDistance: ");
//           Serial.println(currentDistance);
// //
//           Serial.print("**** maxDis: ");
//           Serial.println(maxDis);
//
//        // Serial.print("**** distanceThreshold: ");
//        // Serial.println(distanceThreshold);
//








//        // Serial.print("**** currentDistance - maxDis >= distanceThreshold: ");
//        // Serial.println(abs(currentDistance - maxDis));
//        
       // if(( abs(currentDistance - maxDis) <= distanceThreshold ) && paused == true){
       //  if(abs(currentDistance - maxDis) <= distanceThreshold){
       // //   // Break out of the wait

       //    // this is weird
       //   // if(abs(currentDistance - maxDis) < 5 || distanceThreshold == 400){
       //   //   return;
       //   // }

       //   foundIndex = 0;
       //   mode = 0;

       // //   // Serial.println("**** WE EVER GET HERE?");
       // //   // Always gets there
       //    paused = false;
       //   // return;
       // }

       // looks at the 
       unsigned long waitCurrentMillis = millis();
       // if(waitCurrentMillis - waitPreviousMillis >= waitInterval && paused == false) {
        if(waitCurrentMillis - waitPreviousMillis >= waitInterval || breakout == true) {
         waitPreviousMillis = waitCurrentMillis;
         // Serial.println("GET HERE?!");

         foundIndex = 0;
         mode = 0;
         paused = false;
         breakout = false;

         // Serial.print("MODE: ");
         // Serial.println(mode);
         // Serial.print("foundIndex: ");
         // Serial.println(foundIndex);
       } else if (waitCurrentMillis - waitPreviousMillis < waitInterval) {
        Serial.println("*********** ***** *** When does this happen????");
        paused = true;
       }

       // if(( abs(currentDistance - maxDis) <= distanceThreshold ) && paused == true){
        Serial.print("currentDistance: ");
        Serial.println(currentDistance);

        Serial.print("maxDis: ");
        Serial.println(maxDis);

        // Serial.print("distanceThreshold: ");
        // Serial.println(distanceThreshold);

        // Serial.print("answer: ");
        // Serial.println(abs(currentDistance - maxDis));

        Serial.print("1 answer: ");
        // Serial.println( currentDistance );
        Serial.println( distanceThreshold );
        // Serial.println( abs(maxDis - distanceThreshold) );
        // Serial.println( abs((int)currentDistance - (int)abs(maxDis - distanceThreshold )) );

        Serial.println( abs(maxDis + distanceThreshold) );
        // Serial.println( abs((int)currentDistance - (int)abs(maxDis + distanceThreshold )) );
        Serial.println( abs( (int)currentDistance - (int)abs(maxDis - distanceThreshold ) ) );
        

        // Serial.print("2 answer: ");
        // // Serial.println( currentDistance);
        // Serial.println( abs(maxDis + distanceThreshold) );

        // if(abs(currentDistance - maxDis) <= distanceThreshold){
        // if(( currentDistance >= (maxDis + maxError) - distanceThreshold ) ){



         // #### TODO
         // #### TODO
         // #### TODO
         // #### TODO
         // #### TODO
         // #### TODO
         // #### TODO


          // the paused flag prevents the motors from running cause it returns the loop
          // but if paused is false it skips the return anf runs the motors
          // but i need a way to break out if the distance is different
          // when the motor starts up again
          // the distances are all weird... look into a debounce perhaps 




         // #### TODO
         // #### TODO
         // #### TODO
         // #### TODO
         // #### TODO
         // #### TODO
         // #### TODO
         // #### TODO
         // #### TODO



        
            // THIS IS GOOD!
          if( abs((int)currentDistance - (int)abs(maxDis - distanceThreshold )) >= (int)distanceThreshold )
          //   || (int)currentDistance - (int)abs(maxDis + (distanceThreshold)) )
          // if ( (int)currentDistance - (int)abs(maxDis + (distanceThreshold)) )
          // if ( (int)abs(maxDis + (distanceThreshold)) <= (int)currentDistance )
          {
            // && ( currentDistance) <= maxDis + distanceThreshold) {
              // if(paused == true){
                
                Serial.println("111 ######### BREAK OUT #########  ");
                paused = false;
                breakout = false;
              // }
          } else if((int)abs(maxDis + (distanceThreshold)) <= (int)currentDistance && paused == true){
              Serial.println("222 ######### BREAK OUT #########  ");
              paused = false;
              breakout = false;
          }


        Serial.println("333333333333333333333333333333333333333333");


        if(paused == true){
          Serial.println("44444444444444444444444");
          return;
        }

        Serial.println("55555555555555555555555555");
        
      }


      //  Controls the motor
      if(motorCurrentPosition <= motorMaxPosition && motorDirection == 0){
        // INTERLEAVE
        // MICROSTEP
        myMotor->step(motorStep, FORWARD, MICROSTEP); 
        motorCurrentPosition += motorStep;
      } else if (motorCurrentPosition >= motorStartPosition && motorDirection == 1) {
        myMotor->step(motorStep, BACKWARD, MICROSTEP); 
        motorCurrentPosition -= motorStep;
      }

      // moved to store distance after move
      // int currentStep = abs(motorCurrentPosition) / 2;
      int currentStep = motorCurrentPosition;
      if(motorDirection == 0){
        // Serial.print("currentStep");
        // Serial.println(currentStep);
        sensorArrayValue[currentStep] = currentDistance;
        
        Serial.print("***** CD: ");
        Serial.println(currentDistance);
      } else {
        sensorArrayValue[currentStep] = 0;
      }
//
//      Serial.print("motorCurrentPosition: ");
//      Serial.println(motorCurrentPosition);
//
//      Serial.print("motorDirection: ");
//      Serial.println(motorDirection);

      // Updates the direction of the motor
      if (motorCurrentPosition <= 0){
        // forwards
        motorDirection = 0;
      } else if (motorCurrentPosition >= (motorMaxPosition)) {
        // backwards
        motorDirection = 1;
        mode = 1;
      }

      // previousDistance = currentDistance;
    }
    
  // }


  // Serial.println("######################################");

}

void echoCheck() { // Timer2 interrupt calls this function every 24uS where you can check the ping status.
  // Don't do anything here!
  if (sonar.check_timer()) { // This is how you check to see if the ping was received.
    // Here's where you can add code.
    // Serial.print("Ping: ");
    // Serial.print(sonar.ping_result / US_ROUNDTRIP_CM); // Ping returned, uS result in ping_result, convert to cm with US_ROUNDTRIP_CM.
    // Serial.println("cm");

    currentDistance = sonar.ping_result / US_ROUNDTRIP_CM;
    // int currentStep = abs(motorCurrentPosition) / 2;
    // int currentStep = motorCurrentPosition;
    // Serial.print("currentStep");
    // Serial.println(currentStep);
    // sensorArrayValue[currentStep] = currentDistance;

  }
  // Don't do anything here!
}

// you can change these to DOUBLE or INTERLEAVE or MICROSTEP!
// wrappers for the first motor!
void forwardstep1() {  
  myMotor->onestep(FORWARD, MICROSTEP);
}

void backwardstep1() {  
  myMotor->onestep(BACKWARD, MICROSTEP);
}


