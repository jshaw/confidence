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

// Update it to be a class
// https://learn.adafruit.com/multi-tasking-the-arduino-part-1?view=all

#define DEBUG false

#include <Servo.h>
#include <NewPing.h>
#include <Array.h>

#define TRIGGER_PIN   12 // Arduino pin tied to trigger pin on ping sensor.
#define ECHO_PIN      11 // Arduino pin tied to echo pin on ping sensor.
#define MAX_DISTANCE 400 // Maximum distance we want to ping for (in centimeters). Maximum sensor distance is rated at 400-500cm.

NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE); // NewPing setup of pins and maximum distance.
// twelve servo objects can be created on most boards
Servo myservo;

unsigned int pingSpeed = 50; // How frequently are we going to send out a ping (in milliseconds). 50ms would be 20 times a second.
unsigned long pingTimer;     // Holds the next ping time.

unsigned long previousMillis = 0;
const long interval = 10;


unsigned long distancePreviousMillis = 0;
const long distanceInterval = 5000;


int increment = 1;
int control_increment = 10;

boolean mode = true;
int pos = 0;    // variable to store the servo position

// Setting up the different values for the sonar values while rotating
const byte size = 26;
int sensorArrayValue[size] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25};
Array<int> array = Array<int>(sensorArrayValue, size);

// defaults to stop
int incomingByte = 115;

bool paused = false;

void setup() {
  Serial.begin(115200);
  myservo.attach(9);
  pingTimer = millis();

  pos = 90;
  myservo.write(pos);
}

void loop() {

  // read the incoming byte:
  if (Serial.available() > 0) {
    incomingByte = Serial.read();
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
    pos -= control_increment;
    myservo.write(pos);
    // sets it to stop
    incomingByte = 115;

  // previous
  } else if(incomingByte == 112){
    pos += control_increment;
    myservo.write(pos);
    // sets it to stop
    incomingByte = 115;

  // stop
  } else if(incomingByte == 115){

  // resets position back to 90
  } else if(incomingByte == 99){
    pos = 90;
    myservo.write(pos);
  }

  // stop the loop if it is anything other than 'g' (go)
  if (incomingByte != 103){
    return;
  }
  
  unsigned long currentMillis = millis();
  
  if (currentMillis < 500 || currentMillis >= pingTimer) {   
    pingTimer += pingSpeed;  
    sonar.ping_timer(echoCheck); 
  }

  float dist = sonar.ping_result / US_ROUNDTRIP_CM;

  if(dist < 30 && dist > 5 ){
    if(pos > 90){
      pos = 180;
    } else if(pos <= 90){
      pos = 0;
    }
    
    distancePreviousMillis = millis();
    myservo.write(pos);
    paused = true;
  }

  if((currentMillis - distancePreviousMillis) > distanceInterval){
    paused = false;
  }

  if(paused == true){
    return;
  }

  if((currentMillis - previousMillis) > interval){
    previousMillis = millis();
    
    pos += increment;
    myservo.write(pos);

    // end of sweep
    if ((pos >= 180) || (pos <= 0)){
      // reverse direction
      increment = -increment;
    }
  }
  
  if (pos >= 180 && mode == true){
    mode = false;
  } else if (pos <= 0 && mode == false){
    mode = true;
  }
}

// Timer2 interrupt calls this function every 24uS where you can check the ping status.
void echoCheck() {
  if (sonar.check_timer()) {
    // Here's where you can add code.
    Serial.print("Possition: ");
    Serial.println(pos);
    Serial.print("Ping: ");
    // Ping returned, uS result in ping_result, convert to cm with US_ROUNDTRIP_CM.
    Serial.print(sonar.ping_result / US_ROUNDTRIP_CM);
    Serial.println("cm");

    Serial.println("==============");
    
  }
}

