#include <Arduino.h>
//#include <ArduinoJson.h>
#include <SimplexNoise.h>

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
#include <NewPing.h>
#include <Array.h>

#define MAX_DISTANCE 400 // Maximum distance (in cm) to ping.
#define PING_INTERVAL 33 // Milliseconds between sensor pings (29ms is about the min to avoid cross-sensor echo).

int control_increment = 10;

//boolean mode = true;
// basic: just move back and forth
// pattern: offset basic back and forth based on ID
// patternWave: offset basic back and forth based on position on the board
// patternWaveSmall: offset basic back and forth based on position on the board but less difference between the starting degrees per object
// patternWaveSmall_v2: smaller wave form
// react: react
// reactAndPause: react and pause
String mode = "basic";
int pos = 0;    // variable to store the servo position

// defaults to stop
int incomingByte = 115;

class Sweeper
{
    SimplexNoise sn;

    int pin_cache;

    int buttonPushCounter = 1;
    int min_degree = 0;
    int max_degree = 0;

    double n;
    float increase = 0.01;
    float x = 0.0;
    float y = 0.0;

    int minAngle = 0;
    int maxAngle = 180;
    
    Servo servo;              // the servo
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

    String sweepString = "";

    unsigned long pausedPreviousMillis;
    unsigned long pausedInterval;
    bool paused;

    boolean printJSON = true;
    boolean publish_data = false;

    // number of pings collected
    unsigned long pingTotalCount = 0;
    // number of pings before send for simplexNoise
    unsigned long pingRemainderValue = 50;
    
    // sendJSON can be true or false. Sends over serial or doesn't.
    // Help for debugging buffer limig
//    boolean sendJSON = false;
//    boolean storeDataJSON = false;
//    int arrayIndex;

    // this section is for interaction smoothing
    //===========================
    static const int numReadings = 5;
    // the readings from the analog input
    int readings[numReadings];
    // the index of the current reading
    int readIndex = 0;
    // the running total
    int total = 0;
    // the average
    int average = 0;
    // END ===========================

    // =============
    // these two vars are pure debug variels to control what gets sent over serial
    boolean sendJSON = true;
    boolean storeDataJSON = true;
    boolean printStringTitle = false;
    // =============
    
  public:
    Sweeper(int ide, int interval, NewPing &sonar, int position, String mode)
    {
      mode = mode;
      updateInterval = interval;
//      id = ide;
      // makes sure the ID never gets out of the number of objects
      id = constrain(ide, 0, 13);
      pos = position;
      increment = 2;
      paused = false;

      for (int thisReading = 0; thisReading < numReadings; thisReading++) {
        readings[thisReading] = 0;
      }

      lowPos = 70;
      highPos = 110;
      lowDistance = 30;
      highDistance = 90;

      pausedPreviousMillis = 0;
      pausedInterval = 2000;
      paused = false;
      
    }

    void Attach(int pin)
    {
      // if it is not attached, attach
      // otherwise don't try and re-attach
      if(servo.attached() == 0){
        servo.attach(pin); 
      }

      if(!pin_cache){
        PinCache(pin);
      }


    }

    void PinCache (int pin){
      pin_cache = pin;
    }

    void Detach()
    {
      Serial.print("DETATCHING ");
      servo.detach();
    }

    void SetPos(int startPosition)
    {
      pos = startPosition;
      servo.write(pos);
    }
  
    int isAttached()
    {
      return servo.attached();
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
      currentDistance = d;
      if(d == 0){
        return;
      }
    
      // this if statement is to make sure that it doesn't read wierd values while a bit slow
      // at the top or bottom of the rotation
      if((pos < 170 || pos > 10) || paused == false){
        // this is apart of the smoothing algorithm
        total = total - readings[readIndex];
        readings[readIndex] = d;
        total = total + readings[readIndex];
        readIndex = readIndex + 1;
    
        // if we're at the end of the array...
        if (readIndex >= numReadings) {
          // ...wrap around to the beginning:
          readIndex = 0;
        }
    
        // calculate the average:
        average = total / numReadings;
      }
    
    //    Serial.println("______________");
    //    Serial.println(currentDistance);
    //    Serial.println(average);
    //    Serial.println("===============");
    
      if(storeDataJSON == true){
        StoreData(currentDistance);
      }
    }

    boolean GetPublishDataStatus()
    {
      return publish_data;
    }
  
    String GetPublishData()
    {
      return sweepString;
    }
  
    void ResetPublishDataStatus()
    {
      publish_data = false;
      sweepString = "";
    }

    void SendBatchData() {
      // helping debug the serial buffer issue
      if(sendJSON == true){
          if(sweepString.endsWith("/")){
            int char_index = sweepString.lastIndexOf("/");
            sweepString.remove(char_index);
          }
    
          if(printStringTitle == true){
            Serial.println("");
            Serial.print("sweepString: ");
          }
          Serial.println(sweepString);
          publish_data = true;
      }
    }

    void StoreData(int currentDistance)
    {
      if(printStringTitle == true){
        if(String(currentDistance).length() > 0){
          Serial.print("currentDistance: ");
          Serial.print((String)currentDistance);
          Serial.print(" ||||");
          Serial.println(" ");
        }
      }
  
      if(String(currentDistance).length() > 0){
        String tmp = String(id);
        tmp.concat(":");
        tmp.concat(String(pos));
        tmp.concat(":");
        tmp.concat(String(currentDistance));
        
        sweepString.concat(tmp);
        sweepString.concat("/");
  
        pingTotalCount++;
      }
    }

    void resetScanValues(){
      //jarray = jsonBuffer->createArray();
      //jarray.clear();
    }

    void Update() {

      if (pos == -1) {
        pos = 0;
        servo.write(pos);
      }

      if ((millis() - pausedPreviousMillis) > pausedInterval) {

        // reattach servo
        paused = false;
      }

      if (mode == "basic") {
        // BIG NOTE
        // MORE NOTE// NEED TO PUT THIS BACK IN AT SOME POINT OR REFACTOR
        // modeBasic();

        if((millis() - lastUpdate) > updateInterval)  // time to update
        {
          lastUpdate = millis();
    
          if (buttonPushCounter == 1){
            // Sweep
            min_degree = 0;
            max_degree = 170;
            pos += increment;
          } else  if (buttonPushCounter == 2){
            // Noise
            min_degree = 15;
            max_degree = 155;
    
            n = sn.noise(x, y);
            x += increase;
      
            pos = (int)map(n*100, -100, 100, minAngle, maxAngle);
            
          } else if (buttonPushCounter == 3){
            // sweep interact
            min_degree = 0;
            max_degree = 170;
            
            if (pos > lowPos && pos < highPos) {
              if (average < highDistance && average > lowDistance ) {
                
                if (pos > 90) {
                  pos = 160;
                } else if (pos <= 90) {
                  pos = 10;
                }
    
                // potential put a pause in here..
                // paused = true;
              } else {
                pos += increment;
              }
            } else {
              pos += increment;
            }
            
          } else if (buttonPushCounter == 4) {
            // noise interact
            min_degree = 15;
            max_degree = 155;
    
            if(paused == false){
              n = sn.noise(x, y);
              x += increase;
              
              pos = (int)map(n*100, -100, 100, minAngle, maxAngle);
            }
    
            if(paused == false){
              if (pos > lowPos && pos < highPos) {
                highDistance = 80;
                if (average < highDistance && average > lowDistance ) {
    
                  servo.write(pos+10);
                  delay(100);
                  servo.write(pos-10);
                  delay(100);
                  servo.write(pos);
                  delay(100);
    
                  // potential put a pause in here..
                  pausedPreviousMillis = millis();
                  paused = true;
                } else {
                  // keep empty
                }
              } else {
                // keep empty
              }
            }
          }
    
          // 
          // =================
          if (paused == true) {
            return;
          } else {
            if(servo.attached() == false){
              // TODO... Pass IN THE PIN ID SO REF HERE
              servo.attach(pin_cache);

            }
            servo.write(pos);
          }
    
          if (buttonPushCounter == 1 || buttonPushCounter == 3){
            // sweep
            if ((pos >= max_degree) || (pos <= min_degree)) // end of sweep
            {
              // send data through serial here
              SendBatchData();
              servo.detach();
                // TODO... Pass IN THE PIN ID SO REF HERE
              servo.attach(pin_cache);
              // reverse direction
              increment = -increment;
            }
          } else if (buttonPushCounter == 2 || buttonPushCounter == 4){
            // Noise
            // Send the ping data readings on every nth count
            if(pingTotalCount % pingRemainderValue == 0){
              SendBatchData();
            }
          }

        }


      // This is for the other patterns methods... this needs to be integrated into the above interactions
      } else if (mode == "pattern" || mode == "patternWave" || mode == "patternWaveSmall" || mode == "patternWaveSmall_v2") {
        modePattern();
      } else {
        // this is the react
        if ((millis() - lastUpdate) > updateInterval) // time to update
        {
          lastUpdate = millis();

          if (pos > lowPos && pos < highPos) {
            // if(currentDistance < 100 && currentDistance > 5 ){
            if (currentDistance < highDistance && currentDistance > lowDistance ) {
              if (pos > 90) {
                pos = 170;
              } else if (pos <= 90) {
                pos = 10;
              }

              pausedPreviousMillis = millis();
              paused = true;
              servo.write(pos);
              // detatch servo here
              increment = -increment;
            } else {

              // something is happing here that is a bit funny.
              // It doesn't allow for a full reset to the top or bottom sometimes
              if (paused == true) {
                return;
              }

              pos += increment;
            }
          } else {

            // something is happing here that is a bit funny.
            // It doesn't allow for a full reset to the top or bottom sometimes
            // Or it happens here
            if (paused == true) {
              // check if detatched, if not
              // detatch servo here
              return;
            }

            pos += increment;
          }

          // something is happing here that is a bit funny.
          // It doesn't allow for a full reset to the top or bottom sometimes
          // Or here!
          if (paused == true) {
            // check if detatched, if not
            // detatch servo herer
            return;
          } else {
            int posConstrain = constrain(pos, 10, 170);
            servo.write(pos);

            if ((pos >= 180) || (pos <= 0)) // end of sweep
            {
              // reverse direction
              increment = -increment;
              resetScanValues();
            }
          }
        }

      }// end of else
    }

    void modeBasic()
    {
      if ((millis() - lastUpdate) > updateInterval) // time to update
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
      if ((millis() - lastUpdate) > updateInterval) // time to update
      {
        lastUpdate = millis();
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

#define SONAR_NUM     20 // Number of sensors.

unsigned long pingTimer[SONAR_NUM]; // Holds the times when the next ping should happen for each sensor.
unsigned int cm[SONAR_NUM];         // Where the ping distances are stored.
uint8_t currentSensor = 0;          // Keeps track of which sensor is active.

NewPing sonar[SONAR_NUM] = {     // Sensor object array.
  NewPing(A11, 45, MAX_DISTANCE), // Each sensor's trigger pin, echo pin, and max distance to ping.
  NewPing(A12, 47, MAX_DISTANCE),
  NewPing(A13, 49, MAX_DISTANCE),
  NewPing(A14, 51, MAX_DISTANCE),
  NewPing(A15, 53, MAX_DISTANCE),
  NewPing(52, 33, MAX_DISTANCE),
  NewPing(50, 35, MAX_DISTANCE),
  NewPing(48, 37, MAX_DISTANCE),
  NewPing(46, 39, MAX_DISTANCE),
  NewPing(44, 41, MAX_DISTANCE),
  NewPing(31, 7, MAX_DISTANCE),
  NewPing(29, 6, MAX_DISTANCE),
  NewPing(27, 5, MAX_DISTANCE),
  NewPing(25, 4, MAX_DISTANCE),
  NewPing(23, 3, MAX_DISTANCE),
//  // ---------
  NewPing(9, 2, MAX_DISTANCE),
  NewPing(10, 18, MAX_DISTANCE),
  NewPing(11, 19, MAX_DISTANCE),
  NewPing(12, 20, MAX_DISTANCE),
  NewPing(13, 21, MAX_DISTANCE)
};


//#define OBJECT_NUM  5 // Number of sensors.
//#define OBJECT_NUM_V2  8 // Number of sensors.
//#define OBJECT_NUM_TOTAL  13 // Number of sensors.

#define OBJECT_NUM  20 // Number of sensors.

// Sensor object array.
// ID, Update Interval, Sonar ID, Start Possition, mode
Sweeper sweep[OBJECT_NUM] = {
  Sweeper(0, 20, sonar[0], 0, mode),
  Sweeper(1, 20, sonar[1], 0, mode),
  Sweeper(2, 20, sonar[2], 0, mode),
  Sweeper(3, 20, sonar[3], 0, mode),
  Sweeper(4, 20, sonar[4], 0, mode),
  
  Sweeper(5, 20, sonar[5], 0, mode),
  Sweeper(6, 20, sonar[6], 0, mode),
  Sweeper(7, 20, sonar[7], 0, mode),
  Sweeper(8, 20, sonar[8], 0, mode),
  Sweeper(9, 20, sonar[9], 0, mode),
  Sweeper(10, 20, sonar[10], 0, mode),
  Sweeper(11, 20, sonar[11], 0, mode),
  Sweeper(12, 20, sonar[12], 0, mode),
  Sweeper(13, 20, sonar[13], 0, mode),
  Sweeper(14, 20, sonar[14], 0, mode),
  Sweeper(15, 20, sonar[15], 0, mode),
  Sweeper(16, 20, sonar[16], 0, mode),
  Sweeper(17, 20, sonar[17], 0, mode),
  Sweeper(18, 20, sonar[18], 0, mode),
  Sweeper(19, 20, sonar[19], 0, mode)
};


// ==============
// ==============

void setup() {
   Serial.begin(115200);

//   StaticJsonBuffer<1400> *jsonBuffer;

  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  // First ping starts at 75ms, gives time for the Arduino to chill before starting.
  pingTimer[0] = millis() + 75;
  // Set the starting time for each sensor.
  for (uint8_t i = 1; i < SONAR_NUM; i++) {
    pingTimer[i] = pingTimer[i - 1] + PING_INTERVAL;
  }

  // I like this idea, but the pins aren't all in order...
  // so may have to do this manually
  // =====
  //  int pin = 22;
  //  // Set the starting time for each sensor.
  //  for (uint8_t i = 0; i < OBJECT_NUM; i++) {
  //    sweep[i].Attach(pin);
  //    pin++;
  //  }
  //  sweep[0].Attach(pin);
  // =====// =====// =====// =====

  sweep[0].Attach(A0);
  sweep[1].Attach(A1);
  sweep[2].Attach(A2);
  sweep[3].Attach(A3);
  sweep[4].Attach(A4);
  sweep[5].Attach(A5);
  sweep[6].Attach(A6);
  sweep[7].Attach(A7);
  sweep[8].Attach(A8);
  sweep[9].Attach(A9);
  sweep[10].Attach(40);
  sweep[11].Attach(38);
  sweep[12].Attach(36);
  sweep[13].Attach(34);
  sweep[14].Attach(32);
  sweep[15].Attach(30);
  sweep[16].Attach(28);
  sweep[17].Attach(26);
  sweep[18].Attach(24);
  sweep[19].Attach(22);


  if (mode == "pattern") {
    for (uint8_t i = 0; i < OBJECT_NUM; i++) {
      int mappedPos = ceil(map(i, 0, 4, 0, 180));

      // Commented out oct 30 evenint
      //Serial.println(mappedPos);

      mappedPos = constrain(mappedPos, 1, 179);
      sweep[i].SetPatternPos(mappedPos);
    }
  } else if (mode == "patternWave") {
    sweep[0].SetPatternPos(0);
    sweep[1].SetPatternPos(90);
    sweep[2].SetPatternPos(179);
    sweep[3].SetPatternPos(45);
    sweep[4].SetPatternPos(135);
  } else if (mode == "patternWaveSmall") {

    // 0 20 40 60 80 100 120 140...
    sweep[0].SetPatternPos(40);
    sweep[1].SetPatternPos(60);
    sweep[2].SetPatternPos(80);
    sweep[3].SetPatternPos(100);
    sweep[4].SetPatternPos(120);
    sweep[5].SetPatternPos(140);
    sweep[6].SetPatternPos(160);
    sweep[7].SetPatternPos(140);
    sweep[8].SetPatternPos(160);
    sweep[9].SetPatternPos(178);
    sweep[10].SetPatternPos(0);
    sweep[11].SetPatternPos(120);
    sweep[12].SetPatternPos(40);
    
  } else if (mode == "patternWaveSmall_v2") {

    // 0 20 40 60 80 100 120 140...
    sweep[0].SetPatternPos(50);
    sweep[1].SetPatternPos(60);
    sweep[2].SetPatternPos(70);
    sweep[3].SetPatternPos(80);
    sweep[4].SetPatternPos(90);
    sweep[5].SetPatternPos(100);
    sweep[6].SetPatternPos(110);
    sweep[7].SetPatternPos(120);
    sweep[8].SetPatternPos(130);
    sweep[9].SetPatternPos(40);
    sweep[10].SetPatternPos(30);
    sweep[11].SetPatternPos(20);
    sweep[12].SetPatternPos(10);
  }

  establishContact();

}

void establishContact() {
  while (Serial.available() <= 0) {
    Serial.println('A');   // send a capital A
    delay(300);
  }
}

void loop() {

  // read the incoming byte:
  if (Serial.available() > 0) {
    incomingByte = Serial.read();
  }

//  delay(500);
  // g = 103
  // s = 115
  // n = 110
  // p = 112
  // c = 99

  // go
  if (incomingByte == 103) {
    
    int pin = 22;
    // Set the starting time for each sensor.
    for (uint8_t i = 0; i < OBJECT_NUM; i++) {
//      Serial.println(i);
      //sweep[i].Attach(pin);
      pin++;
    }

  
    // next
  } else if (incomingByte == 110) {
    // sets it to stop
    incomingByte = 115;

    pos -= control_increment;
    for (uint8_t i = 0; i < OBJECT_NUM; i++) {
      if(sweep[i].isAttached() == 0){
        sweep[i].GoTo(pos);
      }
    }
    return;

    // previous
  } else if (incomingByte == 112) {
    // sets it to stop
    incomingByte = 115;

    pos += control_increment;

    for (uint8_t i = 0; i < OBJECT_NUM; i++) {
      sweep[i].GoTo(pos);
    }
    return;

    // stop
  } else if (incomingByte == 115) {

    // detatch all motors to save energy / motor life span
    int pin = 22;
    // Set the starting time for each sensor.
    for (uint8_t i = 0; i < OBJECT_NUM; i++) {
      //sweep[i].Detach();
      pin++;
    }
    return;

    // resets position back to 90
  } else if (incomingByte == 99) {
    Serial.println("//// in reset");
    pos = 90;
    for (uint8_t i = 0; i < OBJECT_NUM; i++) {
      sweep[i].GoTo(pos);
    }
    return;
  } else {
    // run through as a go!
  }

  unsigned long currentMillis = millis();

  for (uint8_t i = 0; i < OBJECT_NUM; i++) {
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

}

// Timer2 interrupt calls this function every 24uS where you can check the ping status.
void echoCheck() {
  if (sonar[currentSensor].check_timer()) {
    cm[currentSensor] = sonar[currentSensor].ping_result / US_ROUNDTRIP_CM;
    int dis = cm[currentSensor];
    sweep[currentSensor].SetDistance(dis);
  }
}

void oneSensorCycle() { // Sensor ping cycle complete, do something with the results.
  // The following code would be replaced with your code that does something with the ping results.
  for (uint8_t i = 0; i < SONAR_NUM; i++) {

    // commented out oct 30 eve
//    Serial.print(i);
//    Serial.print("=");
//    Serial.print(cm[i]);
//    Serial.print("cm ");

    
  }
//  Serial.println();
}

