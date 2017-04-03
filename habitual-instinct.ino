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
// sweep: just move back and forth
// pattern: offset basic back and forth based on ID
// patternWave: offset basic back and forth based on position on the board
// patternWaveSmall: offset basic back and forth based on position on the board but less difference between the starting degrees per object
// patternWaveSmall_v2: smaller wave form
// react: react
// reactAndPause: react and pause

// the mode needs to be rewritten... can also have
// ===========
// sweep
// sweep react
// sweep react pause
// noise
// noise react
// patternWaveSmall_v2: smaller wave form

String mode = "noise";
int pos = 0;    // variable to store the servo position
unsigned long ping_current_millis;

// defaults to stop
int incomingByte = 115;

class Sweeper
{
    SimplexNoise sn;

    unsigned long current_millis;

    int pin_cache;
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
    // unsigned long pingRemainderValue = 50;
    unsigned long pingRemainderValue = 10;

    // this section is for interaction smoothing
    //===========================
    static const int numReadings = 3;
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
    // these two vars are pure debug variels to control what gets sent over serial or doesn't
    // Help for debugging buffer limit
    boolean sendJSON = true;
    boolean storeDataJSON = true;
    boolean printStringTitle = false;
    // =============
    
  public:
    Sweeper(int ide, int interval, NewPing &sonar, int position, String mode, unsigned long pcount)
    {
      mode = mode;
      updateInterval = interval;
      // makes sure the ID never gets out of the number of objects
      id = constrain(ide, 0, 19);
      // id = ide;
      // id = constrain(ide, 0, 13);
      pos = position;
      increment = 2;
      paused = false;
      
      // sets the pingcount offset so it doesn't send all of the scanned data through serial at once.
      pingTotalCount = pcount;

      // sets the smoothing array to base number
      for (int thisReading = 0; thisReading < numReadings; thisReading++) {
        readings[thisReading] = 0;
      }

      lowPos = 70;
      highPos = 110;
      lowDistance = 30;
      highDistance = 100;

      pausedPreviousMillis = 0;
      pausedInterval = 2000;
      paused = false;
      
      x = random(0.0, 20.0);
      
    }

    void Attach(int pin)
    {

      Serial.print("x : ");
      Serial.println(x);
      // if it is not attached, attach
      // otherwise don't try and re-attach
      if(servo.attached() == 0){
        servo.attach(pin); 
      }

      // cache the pin ID in constructure to use later for 
      // attaching and detaching
      if(!pin_cache){
        PinCache(pin);
      }
      
    }

    void PinCache (int pin){
      pin_cache = pin;
    }

    void Detach()
    {
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

    void resetDefaults(){
      increment = 2;
      lastUpdate = millis();
      paused = false;
      pos = 90;
      pausedPreviousMillis = millis();

      // need to reset the reading values
      for (int thisReading = 0; thisReading < numReadings; thisReading++) {
        readings[thisReading] = 0;
      }
    }

    void PrintDistance(int d)
    {
      Serial.print("Print Distance: ");
      Serial.println(d);
      // Serial.println(sonar->ping_result);
      Serial.println("===================");

    }

    void SetDistance(int d)
    {
      currentDistance = d;
      if(currentDistance == 0){
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

      if(mode == "measure"){
        Serial.println("______________");
        Serial.print("pin_cache: ");
        Serial.println(pin_cache);
        Serial.println(currentDistance);
        Serial.println(average);
        Serial.println("===============");
      }
    
      if(storeDataJSON == true){
//        if(isAttached() == true){
          if(paused == false){
            StoreData(currentDistance);
          }
//        }
      }
    }

    void StoreData(int currentDistance)
    {
      // Just some debugging here
      if(printStringTitle == true){
        if(String(currentDistance).length() > 0){
          Serial.print("currentDistance: ");
          Serial.print((String)currentDistance);
          Serial.print(" ||||");
          Serial.println(" ");
        }
      }
  
      if(String(currentDistance).length() > 0){
        if(paused == false){
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
    }

    boolean GetPublishDataStatus()
    {
      return publish_data;
    }
  
    String GetPublishData()
    {
      return sweepString;
    }

    void SendBatchData() {
      // helping debug the serial buffer issue
      if(sendJSON == true){
          if(sweepString.endsWith("/")){
            int char_index = sweepString.lastIndexOf("/");
            sweepString.remove(char_index);
          }
    
          if(printStringTitle == true){
            // Serial.println("");
            // Serial.print("sweepString: ");
          }

          // REMEMBER: THIS IS THE SEND BATCH SERIAL PRINT!!
          //Serial.println(sweepString);
          
          publish_data = true;

          ResetPublishDataStatus();
      }
    }

    void ResetPublishDataStatus()
    {
      publish_data = false;
      sweepString = "";
    }

    void Update() {

      if (pos == -1) {
        pos = 0;
        servo.write(pos);
      }

      current_millis = millis();

      if ((current_millis - pausedPreviousMillis) > pausedInterval) {
        // reattach servo
        paused = false;
      }
      
      if (mode == "sweep") {

        if((current_millis - lastUpdate) > updateInterval)  // time to update
        {
          lastUpdate = millis();
          min_degree = 0;
          max_degree = 170;
          pos += increment;
    
          // 
          // =================
          if (paused == true) {
            return;
          } else {
            if(servo.attached() == false){
              servo.attach(pin_cache);
            }
            servo.write(pos);
          }

          // sweep
          // == BEGINNING OF SWEEP SEND DATA
          if ((pos >= max_degree) || (pos <= min_degree)) // end of sweep
          {
            Detach();
            servo.attach(pin_cache);
            // reverse direction
            increment = -increment;
          }
          // END OF SWEEP SEND DATA

          // ====== BEGINNING OF SWEEP COUND OFFSET DATA
          // if(pingTotalCount % pingRemainderValue == 0){
          if(pingTotalCount >= pingRemainderValue){
            SendBatchData();
            pingTotalCount = 1;
            
          }
          // ====== END OF SWEEP COUND OFFSET DATA

        }

      } else if (mode == "sweep_react"){

        if((current_millis - lastUpdate) > updateInterval)  // time to update
        {
          lastUpdate = millis();

          // sweep interact
          min_degree = 0;
          max_degree = 170;
          
          if (pos > lowPos && pos < highPos) {
            if (average < highDistance && average > lowDistance ) {
              
              if (pos > 90) {
                pos = 170;
              } else if (pos <= 90) {
                pos = 10;
              }
  
              // testing here a short pause to allow the motor to get to it's destination
              // before continuing to move
              pausedPreviousMillis = millis();
              pausedInterval = 50;
              paused = true;
            } else {
              pos += increment;
            }
          } else {
            pos += increment;
          }

          if (paused == true) {
            return;
          } else {
            
            if(servo.attached() == false){
              servo.attach(pin_cache);
            }

            servo.write(pos);
          }

          // == BEGINNING OF SWEEP SEND DATA
          if ((pos >= max_degree) || (pos <= min_degree)) // end of sweep
          {
            // send data through serial here
            Detach();
            servo.attach(pin_cache);
            
            // reverse direction
            increment = -increment;
          }
          // END OF SWEEP SEND DATA

          // ====== BEGINNING OF SWEEP COUND OFFSET DATA
//          if(pingTotalCount % pingRemainderValue == 0){
          if(pingTotalCount >= pingRemainderValue){
            SendBatchData();
            pingTotalCount = 1;
            
          }
          // ====== END OF SWEEP COUND OFFSET DATA

        }

      } else if (mode == "sweep_react_pause"){

        if((current_millis - lastUpdate) > updateInterval)  // time to update
        {
          lastUpdate = millis();

          // sweep interact
          min_degree = 0;
          max_degree = 170;
          
          if (pos > lowPos && pos < highPos) {
            if (average < highDistance && average > lowDistance ) {
              
              if (pos > 90) {
                pos = 170;
              } else if (pos <= 90) {
                pos = 10;
              }
  
              // Pause here
              pausedPreviousMillis = millis();
              pausedInterval = 1000;
              paused = true;
              servo.write(pos);
            } else {
              pos += increment;
            }
          } else {
            pos += increment;
          }

          if (paused == true) {
            return;
          } else {
            if(servo.attached() == false){
              servo.attach(pin_cache);
            }
            servo.write(pos);
          }

          // == BEGINNING OF SWEEP SEND DATA
          if ((pos >= max_degree) || (pos <= min_degree)) // end of sweep
          {
            // send data through serial here
            Detach();
            servo.attach(pin_cache);
            
            // reverse direction
            increment = -increment;
          }
          // END OF SWEEP SEND DATA

          // ====== BEGINNING OF SWEEP COUND OFFSET DATA
          // if(pingTotalCount % pingRemainderValue == 0){
          if(pingTotalCount >= pingRemainderValue){
            SendBatchData();
            pingTotalCount = 1;
          }
          // ====== END OF SWEEP COUND OFFSET DATA

        }

      } else if (mode == "noise"){

        if((current_millis - lastUpdate) > updateInterval)  // time to update
        {
          lastUpdate = millis();

          // Noise
          min_degree = 15;
          max_degree = 155;

          n = sn.noise(x, y);
          x += increase;
    
          pos = (int)map(n*100, -100, 100, minAngle, maxAngle);

          if (paused == true) {
            return;
          } else {
            if(servo.attached() == false){
              servo.attach(pin_cache);

            }
            servo.write(pos);
          }

          // ====== BEGINNING OF SWEEP COUND OFFSET DATA
          // if(pingTotalCount % pingRemainderValue == 0){
          if(pingTotalCount >= pingRemainderValue){
            SendBatchData();
            pingTotalCount = 1;
            
          }
          // ====== END OF SWEEP COUND OFFSET DATA

        }

      } else if (mode == "noise_react"){

        if((current_millis - lastUpdate) > updateInterval)  // time to update
        {
          lastUpdate = millis();

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

          if (paused == true) {
            return;
          } else {
            if(servo.attached() == false){
              servo.attach(pin_cache);

            }
            servo.write(pos);
          }

          // ====== BEGINNING OF SWEEP COUND OFFSET DATA
          // if(pingTotalCount % pingRemainderValue == 0){
          if(pingTotalCount >= pingRemainderValue){
            SendBatchData();
            pingTotalCount = 1;
          }
          // ====== END OF SWEEP COUND OFFSET DATA
        }

      } else if (mode == "patern_wave_small"){

        if((current_millis - lastUpdate) > updateInterval)  // time to update
        {
          lastUpdate = millis();
        }
      } else if (mode == "measure"){
        if((current_millis - lastUpdate) > updateInterval)  // time to update
        {
          lastUpdate = millis();
          Serial.print(pin_cache);
          Serial.print(" : ");
          Serial.print("average: ");
          Serial.println(average);
          
        }

      } else if (mode == "measure_react"){
        if((current_millis - lastUpdate) > updateInterval)  // time to update
        {
          lastUpdate = millis();

          // sweep interact
          min_degree = 0;
          max_degree = 170;
          
          if (pos > lowPos && pos < highPos) {
            if (average < highDistance && average > lowDistance ) {
              
              if (pos > 90) {
                pos = 170;
              } else if (pos <= 90) {
                pos = 10;
              }
  
              // testing here a short pause to allow the motor to get to it's destination
              // before continuing to move
              pausedPreviousMillis = millis();
              pausedInterval = 50;
              paused = true;
            } else {
//              pos += increment;
            }
          } else {
//            pos += increment;
          }
          
        }        
      } else if (mode == "pattern" || mode == "pattern_wave_small_v2") {
      // This is for the other patterns methods... this needs to be integrated into the above interactions
        modePattern();
      } else {

        // You know, nothing is running here!
        // =================
//        Serial.println("wtf???");
      }// end of else
    }

    void setMode (String md){
      mode = md;
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

// Number of sensors.
#define OBJECT_NUM  20

unsigned long pingTimer[OBJECT_NUM]; // Holds the times when the next ping should happen for each sensor.
unsigned int cm[OBJECT_NUM];         // Where the ping distances are stored.
uint8_t currentSensor = 0;          // Keeps track of which sensor is active.

NewPing sonar[OBJECT_NUM] = {     // Sensor object array.
  NewPing(A11, 45, MAX_DISTANCE), 
// Each sensor's trigger pin, echo pin, and max distance to ping.
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
  NewPing(9, 2, MAX_DISTANCE),
  NewPing(10, 18, MAX_DISTANCE),
  NewPing(11, 19, MAX_DISTANCE),
  NewPing(12, 20, MAX_DISTANCE),
  NewPing(13, 21, MAX_DISTANCE)
};

// Sensor object array.
// ID, Update Interval, Sonar ID, Start Possition, mode, ping index offset
int update_interval = 35;
Sweeper sweep[OBJECT_NUM] = {
  Sweeper(0, update_interval, sonar[0], 0, mode, 0),
  Sweeper(1, update_interval, sonar[1], 0, mode, 2),
  Sweeper(2, update_interval, sonar[2], 0, mode, 4),
  Sweeper(3, update_interval, sonar[3], 0, mode, 6),
  Sweeper(4, update_interval, sonar[4], 0, mode, 8),
  Sweeper(5, update_interval, sonar[5], 0, mode, 10),
  Sweeper(6, update_interval, sonar[6], 0, mode, 12),
  Sweeper(7, update_interval, sonar[7], 0, mode, 14),
  Sweeper(8, update_interval, sonar[8], 0, mode, 16),
  Sweeper(9, update_interval, sonar[9], 0, mode, 18),
  Sweeper(10, update_interval, sonar[10], 0, mode, 20),
  Sweeper(11, update_interval, sonar[11], 0, mode, 22),
  Sweeper(12, update_interval, sonar[12], 0, mode, 24),
  Sweeper(13, update_interval, sonar[13], 0, mode, 26),
  Sweeper(14, update_interval, sonar[14], 0, mode, 28),
  Sweeper(15, update_interval, sonar[15], 0, mode, 30),
  Sweeper(16, update_interval, sonar[16], 0, mode, 32),
  Sweeper(17, update_interval, sonar[17], 0, mode, 34),
  Sweeper(18, update_interval, sonar[18], 0, mode, 36),
  Sweeper(19, update_interval, sonar[19], 0, mode, 0)
};


// ==============
// ==============

void setup() {
  Serial.begin(115200);

  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  // First ping starts at 75ms, gives time for the Arduino to chill before starting.
  pingTimer[0] = millis() + 75;
  // Set the starting time for each sensor.
  for (uint8_t i = 1; i < OBJECT_NUM; i++) {
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
//  } else if (mode == "patternWave") {
//    sweep[0].SetPatternPos(0);
//    sweep[1].SetPatternPos(90);
//    sweep[2].SetPatternPos(179);
//    sweep[3].SetPatternPos(45);
//    sweep[4].SetPatternPos(135);
//  } else if (mode == "patternWaveSmall") {
//
//    // 0 20 40 60 80 100 120 140...
//    sweep[0].SetPatternPos(40);
//    sweep[1].SetPatternPos(60);
//    sweep[2].SetPatternPos(80);
//    sweep[3].SetPatternPos(100);
//    sweep[4].SetPatternPos(120);
//    sweep[5].SetPatternPos(140);
//    sweep[6].SetPatternPos(160);
//    sweep[7].SetPatternPos(140);
//    sweep[8].SetPatternPos(160);
//    sweep[9].SetPatternPos(178);
//    sweep[10].SetPatternPos(0);
//    sweep[11].SetPatternPos(120);
//    sweep[12].SetPatternPos(40);
    
  } else if (mode == "pattern_wave_small_v2") {

    int panel = 0;
    if(panel == 0){
      // 0 20 40 60 80 100 120 140...
      sweep[0].SetPatternPos(10);
      sweep[1].SetPatternPos(30);
      sweep[2].SetPatternPos(50);
      sweep[3].SetPatternPos(70);
      
      sweep[4].SetPatternPos(20);
      sweep[5].SetPatternPos(40);
      sweep[6].SetPatternPos(60);
      
      sweep[7].SetPatternPos(10);
      sweep[8].SetPatternPos(30);
      sweep[9].SetPatternPos(50);
      
      sweep[10].SetPatternPos(90);
      sweep[11].SetPatternPos(110);
      sweep[12].SetPatternPos(130);
      
      sweep[13].SetPatternPos(80);
      sweep[14].SetPatternPos(100);
      sweep[15].SetPatternPos(120);
      
      sweep[16].SetPatternPos(70);
      sweep[17].SetPatternPos(90);
      sweep[18].SetPatternPos(110);
      sweep[19].SetPatternPos(130);
    } else if(panel == 1){
      sweep[0].SetPatternPos(150);
      sweep[1].SetPatternPos(170);
      // change direction here
      sweep[2].SetPatternPos(170);
      sweep[3].SetPatternPos(150);
      
      sweep[4].SetPatternPos(160);
      sweep[5].SetPatternPos(179);
      // change direction here
      sweep[6].SetPatternPos(160);
      
      sweep[7].SetPatternPos(150);
      sweep[8].SetPatternPos(170);
      sweep[9].SetPatternPos(170);
      
      sweep[10].SetPatternPos(130);
      sweep[11].SetPatternPos(110);
      sweep[12].SetPatternPos(90);
      
      sweep[13].SetPatternPos(140);
      sweep[14].SetPatternPos(120);
      sweep[15].SetPatternPos(100);
      
      sweep[16].SetPatternPos(150);
      sweep[17].SetPatternPos(130);
      sweep[18].SetPatternPos(110);
      sweep[19].SetPatternPos(90);
    } else if(panel == 2){
      sweep[0].SetPatternPos(70);
      sweep[1].SetPatternPos(50);
      sweep[2].SetPatternPos(30);
      sweep[3].SetPatternPos(10);
      
      sweep[4].SetPatternPos(60);
      sweep[5].SetPatternPos(40);
      sweep[6].SetPatternPos(20);
      
      sweep[7].SetPatternPos(70);
      sweep[8].SetPatternPos(50);
      sweep[9].SetPatternPos(30);
      
      sweep[10].SetPatternPos(20);
      sweep[11].SetPatternPos(40);
      sweep[12].SetPatternPos(60);
      
      sweep[13].SetPatternPos(10);
      sweep[14].SetPatternPos(20);
      sweep[15].SetPatternPos(40);
      
      sweep[16].SetPatternPos(1);
      sweep[17].SetPatternPos(20);
      sweep[18].SetPatternPos(40);
      sweep[19].SetPatternPos(60);
    } else if(panel == 3){
    }
  }

  establishContact();

}

void establishContact() {
  while (Serial.available() <= 0) {
    // send a capital A
    // Serial.println('A');
    delay(300);
  }
}

void loop() {

  // read the incoming byte:
  if (Serial.available() > 0) {
    incomingByte = Serial.read();
     Serial.println("Character: " + incomingByte);
  }

  // Different Key Codes
  // =================
  
  // Go
  // g = 103

  // Stop
  // s = 115

  // Next
  // n = 110

  // Previous
  // p = 112

  // Configure
  // c = 99

  // Mode Controles + hex values
  // 1 = Sweep / 49
  // 2 = sweep react / 50
  // 3 = sweep react pause / 51
  // 4 = noise / 52
  // 5 = noise react / 53
  // 6 = pattern_wave_small_v2: smaller wave form / 54
  // 7 = measure / 55
  // 8 = measure + react only / 56

  // END OF CODES
  // ==================


  // go
  if (incomingByte == 103) {
    
//     int pin = 22;
//     // Set the starting time for each sensor.
//     for (uint8_t i = 0; i < OBJECT_NUM; i++) {
// //      Serial.println(i);
//       //sweep[i].Attach(pin);
//       pin++;
//     }
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
    
    mode = "sweep";
  
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
    mode = "stop";
    pos = 90;
    for (uint8_t i = 0; i < OBJECT_NUM; i++) {
      sweep[i].GoTo(pos);
      sweep[i].resetDefaults();
    }
    delay(500);
    massDetatch();
    return;

    // resets position back to 90
    // 'c'
  } else if (incomingByte == 99) {
    pos = 90;
    for (uint8_t i = 0; i < OBJECT_NUM; i++) {
      sweep[i].GoTo(pos);
      sweep[i].resetDefaults();
    }
    return;
  } else if (incomingByte == 49) {
    mode = "sweep";
    for (uint8_t i = 0; i < OBJECT_NUM; i++) {
     sweep[i].setMode(mode);
     sweep[i].resetDefaults();
    }

  } else if (incomingByte == 50) {
    mode = "sweep_react";
    Serial.println("mode: ????????????");
    Serial.println(mode);
    for (uint8_t i = 0; i < OBJECT_NUM; i++) {
     sweep[i].setMode(mode);
    }
  } else if (incomingByte == 51) {
    // sweep react paulse
    mode = "sweep_react_pause";
    for (uint8_t i = 0; i < OBJECT_NUM; i++) {
     sweep[i].setMode(mode);
     sweep[i].resetDefaults();
    }
  } else if (incomingByte == 52) {
    mode = "noise";
    for (uint8_t i = 0; i < OBJECT_NUM; i++) {
     sweep[i].setMode(mode);
     sweep[i].resetDefaults();
    }
  } else if (incomingByte == 53) {
    mode = "noise_react";
    for (uint8_t i = 0; i < OBJECT_NUM; i++) {
     sweep[i].setMode(mode);
     sweep[i].resetDefaults();
    }
  } else if (incomingByte == 54) {
    mode = "pattern_wave_small_v2";
    for (uint8_t i = 0; i < OBJECT_NUM; i++) {
     sweep[i].setMode(mode);
     sweep[i].resetDefaults();
    }

    setPatternWavePosition();
  } else if (incomingByte == 55) {
    mode = "measure";
    massDetatch();
    for (uint8_t i = 0; i < OBJECT_NUM; i++) {
     sweep[i].setMode(mode);
     sweep[i].resetDefaults();
    }
  } else if (incomingByte == 56) {
    mode = "measure_react";
    for (uint8_t i = 0; i < OBJECT_NUM; i++) {
     sweep[i].setMode(mode);
     sweep[i].resetDefaults();
    }
  } else if (incomingByte == 45) {
    // "-"
    // reset to position to 90, 
    // plus keep the same mode
    pos = 90;
    for (uint8_t i = 0; i < OBJECT_NUM; i++) {
      sweep[i].GoTo(pos);
      sweep[i].resetDefaults();
    }
    delay(1000);
  } else if (incomingByte == 61) {
    // "=" 
    mode = "reset_with_pause";
    pos = 90;
    for (uint8_t i = 0; i < OBJECT_NUM; i++) {
      sweep[i].GoTo(pos);
      sweep[i].resetDefaults();
    }
    delay(500);
    massDetatch();
    // return;
  } else {
    // run through as a go!
  }

  // reset the incomingByte to a non-conditional so the if statement doesn't continously execute
  incomingByte = 0;

  for (uint8_t i = 0; i < OBJECT_NUM; i++) {
    sweep[i].Update();

    ping_current_millis = millis();

    if (ping_current_millis >= pingTimer[i]) {         // Is it this sensor's time to ping?
      // sweep[i].PrintDistance();
      pingTimer[i] += PING_INTERVAL * OBJECT_NUM;  // Set next time this sensor will be pinged.
      if (i == 0 && currentSensor == OBJECT_NUM - 1) oneSensorCycle(); // Sensor ping cycle complete, do something with the results.
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
  for (uint8_t i = 0; i < OBJECT_NUM; i++) {

    // commented out oct 30 eve
//    Serial.print(i);
//    Serial.print("=");
//    Serial.print(cm[i]);
//    Serial.print("cm ");

    
  }
}

// detatch all servos
void massDetatch() {
  // detatch all motors to save energy / motor life span
//  sweep[0].Detach();
//  sweep[1].Detach();
//  sweep[2].Detach();
//  sweep[3].Detach();
//  sweep[4].Detach();
//  sweep[5].Detach();
//  sweep[6].Detach();
//  sweep[7].Detach();
//  sweep[8].Detach();
//  sweep[9].Detach();
//  sweep[10].Detach();
//  sweep[11].Detach();
//  sweep[12].Detach();
//  sweep[13].Detach();
//  sweep[14].Detach();
//  sweep[15].Detach();
//  sweep[16].Detach();
//  sweep[17].Detach();
//  sweep[18].Detach();
//  sweep[19].Detach();

  // Set the starting time for each sensor.
  for (uint8_t i = 0; i < OBJECT_NUM; i++) {
    sweep[i].Detach();
  }
}

void setPatternWavePosition(){
  int panel = 0;
  if(panel == 0){
    // 0 20 40 60 80 100 120 140...
    sweep[0].SetPatternPos(10);
    sweep[1].SetPatternPos(30);
    sweep[2].SetPatternPos(50);
    sweep[3].SetPatternPos(70);
    
    sweep[4].SetPatternPos(20);
    sweep[5].SetPatternPos(40);
    sweep[6].SetPatternPos(60);
    
    sweep[7].SetPatternPos(10);
    sweep[8].SetPatternPos(30);
    sweep[9].SetPatternPos(50);
    
    sweep[10].SetPatternPos(90);
    sweep[11].SetPatternPos(110);
    sweep[12].SetPatternPos(130);
    
    sweep[13].SetPatternPos(80);
    sweep[14].SetPatternPos(100);
    sweep[15].SetPatternPos(120);
    
    sweep[16].SetPatternPos(70);
    sweep[17].SetPatternPos(90);
    sweep[18].SetPatternPos(110);
    sweep[19].SetPatternPos(130);
  } else if(panel == 1){
    sweep[0].SetPatternPos(150);
    sweep[1].SetPatternPos(170);
    // change direction here
    sweep[2].SetPatternPos(170);
    sweep[3].SetPatternPos(150);
    
    sweep[4].SetPatternPos(160);
    sweep[5].SetPatternPos(179);
    // change direction here
    sweep[6].SetPatternPos(160);
    
    sweep[7].SetPatternPos(150);
    sweep[8].SetPatternPos(170);
    sweep[9].SetPatternPos(170);
    
    sweep[10].SetPatternPos(130);
    sweep[11].SetPatternPos(110);
    sweep[12].SetPatternPos(90);
    
    sweep[13].SetPatternPos(140);
    sweep[14].SetPatternPos(120);
    sweep[15].SetPatternPos(100);
    
    sweep[16].SetPatternPos(150);
    sweep[17].SetPatternPos(130);
    sweep[18].SetPatternPos(110);
    sweep[19].SetPatternPos(90);
  } else if(panel == 2){
    sweep[0].SetPatternPos(70);
    sweep[1].SetPatternPos(50);
    sweep[2].SetPatternPos(30);
    sweep[3].SetPatternPos(10);
    
    sweep[4].SetPatternPos(60);
    sweep[5].SetPatternPos(40);
    sweep[6].SetPatternPos(20);
    
    sweep[7].SetPatternPos(70);
    sweep[8].SetPatternPos(50);
    sweep[9].SetPatternPos(30);
    
    sweep[10].SetPatternPos(20);
    sweep[11].SetPatternPos(40);
    sweep[12].SetPatternPos(60);
    
    sweep[13].SetPatternPos(10);
    sweep[14].SetPatternPos(20);
    sweep[15].SetPatternPos(40);
    
    sweep[16].SetPatternPos(1);
    sweep[17].SetPatternPos(20);
    sweep[18].SetPatternPos(40);
    sweep[19].SetPatternPos(60);
  } else if(panel == 3){
  }
}

