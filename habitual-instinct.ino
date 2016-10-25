#include <Arduino.h>

// ---------------------------------------------------------------------------
// Habitual Instinct
// By: Jordan Shaw
// Libs: NewPing, Servo, Array
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
//#include <ServoTimer2.h>
#include <NewPing.h>
#include <Array.h>

#define TRIGGER_PIN   12 // Arduino pin tied to trigger pin on ping sensor.
#define ECHO_PIN      11 // Arduino pin tied to echo pin on ping sensor.
#define MAX_DISTANCE 400 // Maximum distance we want to ping for (in centimeters). Maximum sensor distance is rated at 400-500cm.

int control_increment = 10;

//boolean mode = true;
// basic: just move back and forth
// pattern: offset basic back and forth based on ID
// patternWave: offset basic back and forth based on position on the board
// patternWaveSmall: offset basic back and forth based on position on the board but less difference between the starting degrees per object
// react: react
// reactAndPause: react and pause
String mode = "else";
int pos = 0;    // variable to store the servo position

// Setting up the different values for the sonar values while rotating
const byte size = 26;
int sensorArrayValue[size] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25};
Array<int> array = Array<int>(sensorArrayValue, size);

// defaults to stop
int incomingByte = 115;

class Sweeper
{
  Servo servo;              // the servo
//  ServoTimer2 servo;
  int pos;              // current servo position 
  int increment;        // increment to move for each interval
  int  updateInterval;      // interval between updates
  unsigned long lastUpdate; // last update of position
  NewPing *sonar;
  int currentDistance;
  int id;
  int lowPos;
  int highPos;
  int lowDistance;
  int highDistance;
  unsigned long distancePreviousMillis;
  unsigned long distanceInterval;
  bool paused;
 
public: 
  Sweeper(int ide, int interval, NewPing &sonar, int position, String mode)
  {
    mode = mode;
    updateInterval = interval;
    id = ide;
    pos = position;
    increment = 1;
    distancePreviousMillis = 0;
    distanceInterval = 5000;
    paused = false;

    if(id >=3){
      lowPos = 70;
      highPos = 145;
      lowDistance = 20;
      highDistance = 80;
    } else {
      lowPos = 60;
      highPos = 170;
      lowDistance = 10;
      highDistance = 100;
    }
  }
  
  void Attach(int pin)
  {

    Serial.print("ID: ");
    Serial.println(id);
    
    servo.attach(pin);
  }
  
  void Detach()
  {
    servo.detach();
  }

  void GoTo(int pos)
  {
    servo.write(pos);
  }

  void PrintDistance(int d)
  {
    Serial.print("Print Distance: ");
    Serial.println(d);    
//    Serial.println(sonar->ping_result);
    Serial.println("===================");
    
  }

  void SetDistance(int d)
  {
    if(id == 1){
      Serial.print("Class Int D: ");
      Serial.println(d);
    }
    currentDistance = d;
  }
  
  void Update(){
    
    if(pos == -1){
      pos = 0;  
      servo.write(pos);
    }

    if((millis() - distancePreviousMillis) > distanceInterval){
      paused = false;
    }

    if (mode == "basic"){
      modeBasic();
    } else if (mode == "pattern" || mode == "patternWave" || mode == "patternWaveSmall"){
      modePattern();
    } else {
      if((millis() - lastUpdate) > updateInterval)  // time to update
      {
        lastUpdate = millis();
  
  //      Serial.print("currentDistance: ");
  //      Serial.println(currentDistance);
  //      if(pos == -1){
  //        pos = 0;  
  //        servo.write(pos);
  //      }

        if (pos > lowPos && pos < highPos){ 
        // if(currentDistance < 100 && currentDistance > 5 ){
          if(currentDistance < highDistance && currentDistance > lowDistance ){
            if(pos > 90){
              pos = 170;
            } else if(pos <= 90){
              pos = 10;
            }
  
            distancePreviousMillis = millis();
            paused = true;
            servo.write(pos);
            increment = -increment;
          } else {
            
            // something is happing here that is a bit funny.
            // It doesn't allow for a full reset to the top or bottom sometimes
            if (paused == true){
              return;  
            }
          
            pos += increment;
          }
        } else {
  
          // something is happing here that is a bit funny.
          // It doesn't allow for a full reset to the top or bottom sometimes
          // Or it happens here
          if (paused == true){
            return;  
          }
          
          pos += increment;  
        }
  
        // something is happing here that is a bit funny.
        // It doesn't allow for a full reset to the top or bottom sometimes
        // Or here!
        if(paused == true){
          return;
        } else {
  //        Serial.print("Pos: ");
  //        Serial.println(pos);
          int posConstrain = constrain(pos, 10, 170);
          servo.write(pos);
          
          if ((pos >= 180) || (pos <= 0)) // end of sweep
          {
            // reverse direction
            increment = -increment;
          }
        }
      }

    }// end of else
  }

  void modeBasic()
  {
    if((millis() - lastUpdate) > updateInterval)  // time to update
    {
      lastUpdate = millis();
      pos += increment;
      servo.write(pos);
      if ((pos >= 180) || (pos <= 0)) // end of sweep
      {
        // reverse direction
        increment = -increment;
      }
    }
  }

  void SetPatternPos(int p)
  {
    pos = p;
    servo.write(pos);
  }

  // Want to offset movement here...
  // could be done with the 
  void modePattern()
  {
    if((millis() - lastUpdate) > updateInterval)  // time to update
    {
      lastUpdate = millis();
      Serial.println();
      Serial.print("ID Pattern: ");
      Serial.println(id);
      Serial.println("===========");
      pos += increment;
      servo.write(pos);
      if ((pos >= 181) || (pos <= 0)) // end of sweep
      {
        // reverse direction
        increment = -increment;
      }
    }
  }
  
}; // end of class

#define SONAR_NUM     5 // Number of sensors.
#define MAX_DISTANCE 400 // Maximum distance (in cm) to ping.
#define PING_INTERVAL 33 // Milliseconds between sensor pings (29ms is about the min to avoid cross-sensor echo).

unsigned long pingTimer[SONAR_NUM]; // Holds the times when the next ping should happen for each sensor.
unsigned int cm[SONAR_NUM];         // Where the ping distances are stored.
uint8_t currentSensor = 0;          // Keeps track of which sensor is active.

NewPing sonar[SONAR_NUM] = {     // Sensor object array.
  NewPing(8, 9, MAX_DISTANCE), // Each sensor's trigger pin, echo pin, and max distance to ping.
  NewPing(10, 11, MAX_DISTANCE),
  NewPing(12, 13, MAX_DISTANCE),
  NewPing(A0, A1, MAX_DISTANCE),
  NewPing(A2, A3, MAX_DISTANCE)
};


#define OBJECT_NUM  5 // Number of sensors.

// Sensor object array.
// ID, Update Interval, Sonar ID, Start Possition, 
Sweeper sweep[OBJECT_NUM] = {
  Sweeper(0, 10, sonar[0], 90, mode),
  Sweeper(1, 10, sonar[1], 90, mode),
  Sweeper(2, 10, sonar[2], 90, mode),
  Sweeper(3, 10, sonar[3], 90, mode),
  Sweeper(4, 10, sonar[4], 90, mode)
};

// ==============

// Trying to dynamically create sweeper array of objects... maybe not done easily
// http://forum.arduino.cc/index.php?topic=184733.0

//Sweeper sweep[OBJECT_NUM] = {};
//myClass *p[16];
//Sweeper *sweep[OBJECT_NUM]; 
//
//for (uint8_t i = 0; i < OBJECT_NUM; i++){
//  Sweeper sweep[OBJECT_NUM] = Sweeper(10)
//  sweep[OBJECT_NUM] = new Sweeper(10)
//}

// ==============

void setup() {
  Serial.begin(115200);
  
  // First ping starts at 75ms, gives time for the Arduino to chill before starting.
  pingTimer[0] = millis() + 75;
  // Set the starting time for each sensor.
  for (uint8_t i = 1; i < SONAR_NUM; i++){
    pingTimer[i] = pingTimer[i - 1] + PING_INTERVAL;
  }

  int pin = 3;
  // Set the starting time for each sensor.
  for (uint8_t i = 0; i < OBJECT_NUM; i++){
    sweep[i].Attach(pin);
    pin++;
  }

  if(mode == "pattern"){
    for (uint8_t i = 0; i < OBJECT_NUM; i++){
      int mappedPos = ceil(map(i, 0, 4, 0, 180));
      Serial.println(mappedPos);
      mappedPos = constrain(mappedPos, 1, 179);
      sweep[i].SetPatternPos(mappedPos);
    }  
  } else if (mode == "patternWave"){
    sweep[0].SetPatternPos(0);
    sweep[1].SetPatternPos(90);
    sweep[2].SetPatternPos(179);
    sweep[3].SetPatternPos(45);
    sweep[4].SetPatternPos(135);
  } else if (mode == "patternWaveSmall"){

    // 0 20 40 60 80
    sweep[0].SetPatternPos(0);
    sweep[1].SetPatternPos(40);
    sweep[2].SetPatternPos(80);
    sweep[3].SetPatternPos(20);
    sweep[4].SetPatternPos(60);
  }
  
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
    // sets it to stop
    incomingByte = 115;

    pos -= control_increment;
    for (uint8_t i = 0; i < OBJECT_NUM; i++){
      sweep[i].GoTo(pos);
    }

  // previous
  } else if(incomingByte == 112){
    // sets it to stop
    incomingByte = 115;
    
    pos += control_increment;

    for (uint8_t i = 0; i < OBJECT_NUM; i++){
      sweep[i].GoTo(pos);
    }

  // stop
  } else if(incomingByte == 115){

  // resets position back to 90
  } else if(incomingByte == 99){
    pos = 90;
    for (uint8_t i = 0; i < OBJECT_NUM; i++){
      sweep[i].GoTo(pos);
    }
  }

  // stop the loop if it is anything other than 'g' (go)
  if (incomingByte != 103){
    return;
  }
  
  unsigned long currentMillis = millis();

  for (uint8_t i = 0; i < OBJECT_NUM; i++){
    sweep[i].Update();

    if (millis() >= pingTimer[i]) {         // Is it this sensor's time to ping?
//      sweep[i].PrintDistance();
      pingTimer[i] += PING_INTERVAL * SONAR_NUM;  // Set next time this sensor will be pinged.
      if (i == 0 && currentSensor == SONAR_NUM - 1) oneSensorCycle(); // Sensor ping cycle complete, do something with the results.
      sonar[currentSensor].timer_stop();          // Make sure previous timer is canceled before starting a new ping (insurance).
      currentSensor = i;                          // Sensor being accessed.
      cm[currentSensor] = 0;                      // Make distance zero in case there's no ping echo for this sensor.
      sonar[currentSensor].ping_timer(echoCheck); // Do the ping (processing continues, interrupt will call echoCheck to look for echo).
    }
  }
  
//  if (currentMillis < 500 || currentMillis >= pingTimer) {   
//    pingTimer += pingSpeed;  
//    sonar.ping_timer(echoCheck); 
//  }
//
//  float dist = sonar.ping_result / US_ROUNDTRIP_CM;
//
//  if(dist < 30 && dist > 5 ){
//    if(pos > 90){
//      pos = 180;
//    } else if(pos <= 90){
//      pos = 0;
//    }
//    
//    distancePreviousMillis = millis();
//    myservo.write(pos);
//    paused = true;
//  }
//
//  if((currentMillis - distancePreviousMillis) > distanceInterval){
//    paused = false;
//  }
//
//  if(paused == true){
//    return;
//  }
//
//  if((currentMillis - previousMillis) > interval){
//    previousMillis = millis();
//    
//    pos += increment;
//    myservo.write(pos);
//
//    // end of sweep
//    if ((pos >= 180) || (pos <= 0)){
//      // reverse direction
//      increment = -increment;
//    }
//  }
//  
//  if (pos >= 180 && mode == true){
//    mode = false;
//  } else if (pos <= 0 && mode == false){
//    mode = true;
//  }
}

// Timer2 interrupt calls this function every 24uS where you can check the ping status.
void echoCheck() {
//  if (sonar.check_timer()) {
//    // Here's where you can add code.
//    Serial.print("Possition: ");
//    Serial.println(pos);
//    Serial.print("Ping: ");
//    // Ping returned, uS result in ping_result, convert to cm with US_ROUNDTRIP_CM.
//    Serial.print(sonar.ping_result / US_ROUNDTRIP_CM);
//    Serial.println("cm");
//
//    Serial.println("==============");
//    
//  }

  if (sonar[currentSensor].check_timer()){
    cm[currentSensor] = sonar[currentSensor].ping_result / US_ROUNDTRIP_CM;
     int dis = cm[currentSensor];
    
//    if(currentSensor == 1){
//      Serial.print("Echo Check dis: ID");
//      Serial.print(currentSensor);
//      Serial.print(" / Distance: ");
//      Serial.println(dis);
//    }
    sweep[currentSensor].SetDistance(dis);  
    
  }
}

void oneSensorCycle() { // Sensor ping cycle complete, do something with the results.
  // The following code would be replaced with your code that does something with the ping results.
  for (uint8_t i = 0; i < SONAR_NUM; i++) {
    Serial.print(i);
    Serial.print("=");
    Serial.print(cm[i]);
    Serial.print("cm ");
  }
  Serial.println();
}

