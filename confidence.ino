// ---------------------------------------------------------------------------
// Example NewPing library sketch that does a ping about 20 times per second.
// ---------------------------------------------------------------------------

#include <NewPing.h>

#include <Wire.h>
#include <Adafruit_MotorShield.h>
#include "utility/Adafruit_MS_PWMServoDriver.h"

#define TRIGGER_PIN  12  // Arduino pin tied to trigger pin on the ultrasonic sensor.
#define ECHO_PIN     11  // Arduino pin tied to echo pin on the ultrasonic sensor.
#define MAX_DISTANCE 200 // Maximum distance we want to ping for (in centimeters). Maximum sensor distance is rated at 400-500cm.

const long interval = 100;
unsigned long previousMillis = 0;

int currentDistance = 0;
int previousDistance = 0;

NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE); // NewPing setup of pins and maximum distance.

// Create the motor shield object with the default I2C address
Adafruit_MotorShield AFMS = Adafruit_MotorShield(); 

// Connect a stepper motor with 50 steps per revolution (7.2 degree)
// to motor port #2 (M3 and M4)
Adafruit_StepperMotor *myMotor = AFMS.getStepper(100, 1);

void setup() {
  // Open serial monitor at 115200 baud to see ping results.
  Serial.begin(115200);
  // Serial.begin(9600);

  // create with the default frequency 1.6KHz
  AFMS.begin();
  // 10 rpm   
  myMotor->setSpeed(50);
}

void loop() {
  
  unsigned long currentMillis = millis();
  
  if(currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    currentDistance = sonar.ping_cm();
     
    if(currentDistance != 0){
      Serial.print("Ping: ");
      Serial.print(currentDistance); // Send ping, get distance in cm and print result (0 = outside set distance range)
      Serial.println("cm");
    }

    previousDistance = currentDistance;
  }

  Serial.println("Interleave coil steps");
  myMotor->step(100, FORWARD, MICROSTEP); 
  myMotor->step(100, BACKWARD, MICROSTEP); 

  
//  delay(100);                     // Wait 50ms between pings (about 20 pings/sec). 29ms should be the shortest delay between pings.
//  Serial.print("Ping: ");
//  Serial.print(sonar.ping_cm()); // Send ping, get distance in cm and print result (0 = outside set distance range)
//  Serial.println("cm");
}
