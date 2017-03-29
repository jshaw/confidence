#include <Arduino.h>
#include <ArduinoJson.h>

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

//#define TRIGGER_PIN   12 // Arduino pin tied to trigger pin on ping sensor.
//#define ECHO_PIN      11 // Arduino pin tied to echo pin on ping sensor.
#define MAX_DISTANCE 400 // Maximum distance we want to ping for (in centimeters). Maximum sensor distance is rated at 400-500cm.

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

// Setting up the different values for the sonar values while rotating
//const byte size = 26;
//int sensorArrayValue[size] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25};
//Array<int> array = Array<int>(sensorArrayValue, size);

// defaults to stop
int incomingByte = 115;

class Sweeper
{
    Servo servo;              // the servo
//    StaticJsonBuffer StaticJsonBuffer;
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
    boolean printJSON = true;
    // sendJSON can be true or false. Sends over serial or doesn't.
    // Help for debugging buffer limig
    boolean sendJSON = false;
    boolean storeDataJSON = false;
    int arrayIndex;
    
//    StaticJsonBuffer<1400> *jsonBuffer;
//    JsonObject& root = jsonBuffer->createObject();
//    JsonArray& data = root.createNestedArray("data");

    //  const byte size = 26;
    //  int sensorArrayValue[size] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25};

    const byte size = 21;
    //  int sensorArrayValue[size] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19};
    
//    int sensorArrayValue[21] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20};
    String sensorArrayValue[21] = {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "13", "14", "15", "16", "17", "18", "19", "20"};
    Array<String> array = Array<String>(sensorArrayValue, size);

    //  int incrementArray_lg[31] = {0, 6, 12, 18, 24, 30, 36, 42, 48, 54, 60, 66, 72, 78, 84, 90, 96, 102, 108, 114, 120, 126, 132, 138, 144, 150, 156, 162, 168, 174, 180};
//    int incrementArray_sm[21] = {0, 9, 18, 27, 36, 45, 54, 63, 72, 81, 90, 99, 108, 117, 126, 135, 144, 153, 162, 171, 180};

  public:
//    StaticJsonBuffer<1400> jsonBuffer;
//    JsonArray& jarray = jsonBuffer.createArray();
    Sweeper(int ide, int interval, NewPing &sonar, int position, String mode)
    {
      mode = mode;
      updateInterval = interval;
//      id = ide;
      // makes sure the ID never gets out of the number of objects
      id = constrain(ide, 0, 13);
      pos = position;
      increment = 2;
      distancePreviousMillis = 0;
      distanceInterval = 5000;
      paused = false;
      arrayIndex = 0;

      //    const byte size = 26;
      //int sensorArrayValue[size] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25};
      //Array<int> array = Array<int>(sensorArrayValue, size);

//      if (id >= 3) {
        lowPos = 70;
        highPos = 110;
//        highPos = 145;
        lowDistance = 30;
//        highDistance = 80;
        highDistance = 120;
//      } else {
//        lowPos = 60;
//        highPos = 170;
//        lowDistance = 10;
////        highDistance = 100;
//        highDistance = 120;
//      }
    }

    void Attach(int pin)
    {
      // if it is not attached, attach
      // otherwise don't try and re-attach
      if(servo.attached() == 0){

        int idc = constrain(id, 0, 13);
        Serial.print("ID: ");
        Serial.print(id);
        Serial.print(" idc: ");
        Serial.println(idc);
        
        servo.attach(pin); 
      }
    }

    void Detach()
    {
      Serial.print("DETATCHING ");
//      servo.detach();
    }

    void GoTo(int pos)
    {
      servo.write(pos);
    }

    int isAttached()
    {
      return servo.attached();
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
      if (id == 1) {
//        Serial.print("Class Int D: ");
//        Serial.println(d);
      }
      currentDistance = d;

      if(storeDataJSON == true){
        Serial.println("===================");
        StoreData(currentDistance);
      }
    }

    void SendData()
    {
      StaticJsonBuffer<1400> jsonBuffer;
      JsonArray& jarray = jsonBuffer.createArray();

      // TODO:
      // NEED TO READ all of the 

      for(int i = 0; i < array.size(); i++){
        JsonObject& data = jarray.createNestedObject(); 
        String stringToSplit = array[i];
        int commaIndex = stringToSplit.indexOf('/');
        int secondCommaIndex = stringToSplit.indexOf('/', commaIndex+1);
        String firstValue = stringToSplit.substring(0, commaIndex);
        String secondValue = stringToSplit.substring(commaIndex+1, secondCommaIndex);
        String thirdValue = stringToSplit.substring(secondCommaIndex+1); //To the end of the string  
//        data["id"] = var;
//        data["ang"] = (float)var * 6.9;
//        data["dst"] = (int)random(0, 350); 

          data["id"] = firstValue;
          data["ang"] = secondValue;
          data["dst"] = thirdValue;

      }

//      Serial.write("---");
//      if (printJSON == true) {
//
////        String d = "";
//        String d = "[";
//        d.concat(id);
//        d.concat(",");
//        int var = 0;
//        //  while(var < 10){
//        while (var < array.size()) {
//
//          d.concat(array[var]);
//          
//          if(var != (array.size()-1)){
//            d.concat(",");
//          }
//          
//          var++;
//        }

      // helping debug the serial buffer issue
      if(sendJSON == true){
        if (printJSON == true) {
          jarray.printTo(Serial);
          Serial.println("");
  //        jsonBuffer.clear();
  //        jsonBuffer = StaticJsonBuffer<1400>();
  //        JsonArray& jarray = jsonBuffer.createArray();
  //        clear(jsonBuffer);
          printJSON = false;
          arrayIndex = 0;
        }
      }



        //      Serial.println(jarray);
        //        Serial.println(" ");
        //        Serial.println(" ");
        //        Serial.println(" ");
        ////        Serial.print(array[var]);
        //        jarray.printTo(Serial);
        //        Serial.println(" ");
        //        Serial.println(" ");
        //        Serial.println(" ");
//        d.concat("]");
//        Serial.write(" ");
//        Serial.println("");
//        Serial.println(d);
//        printJSON = false;
//                delay(1000);

//      }


    }

    void StoreData(int currentDistance)
    {
//        JsonObject& data = jarray.createNestedObject();

//        Serial.println(currentDistance);

//        data["id"] = id;
//        data["ang"] = (float)pos;
//        data["dst"] = (int)currentDistance;
//        data["tme"] = millis();

        String tmp = (String)id;
        tmp.concat("/");
        tmp.concat((String)pos);
        tmp.concat("/");
        tmp.concat((String)currentDistance);
        
        array[arrayIndex] = tmp;
        arrayIndex++;

//        Serial.println(data);
//        data.printTo(Serial);
      
//      Serial.print("ID: ");
//      Serial.print(id);
//      Serial.print("_____");
//      Serial.println(pos);
//      if (
//        (pos == 0)
//        || (pos == 9)
//        || (pos == 18)
//        || (pos == 27)
//        || (pos == 36)
//        || (pos == 45)
//        || (pos == 54)
//        || (pos == 63)
//        || (pos == 72)
//        || (pos == 81)
//        || (pos == 90)
//        || (pos == 99)
//        || (pos == 108)
//        || (pos == 117)
//        || (pos == 126)
//        || (pos == 135)
//        || (pos == 144)
//        || (pos == 153)
//        || (pos == 162)
//        || (pos == 171)
//        || (pos == 180)
//      ) {
//        int idx = 180 / pos;
////        Serial.print("IDX: ");
////        Serial.println(idx);
//        sensorArrayValue[idx] = currentDistance;
//      }

//      Serial.print("pos: ");
//      Serial.println(pos);

//      Serial.write("pos: ");
//      Serial.write(pos);

//      if (pos == 10 || pos == 170){
      if (pos < 5 || pos > 175){
        printJSON = true;
        SendData();
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

      if ((millis() - distancePreviousMillis) > distanceInterval) {

        // reattach servo
        paused = false;
      }

      if (mode == "basic") {
        modeBasic();
      } else if (mode == "pattern" || mode == "patternWave" || mode == "patternWaveSmall" || mode == "patternWaveSmall_v2") {
        modePattern();
      } else {
        // this is the react
        if ((millis() - lastUpdate) > updateInterval) // time to update
        {
          lastUpdate = millis();

          //      Serial.print("currentDistance: ");
          //      Serial.println(currentDistance);
          //      if(pos == -1){
          //        pos = 0;
          //        servo.write(pos);
          //      }

          if (pos > lowPos && pos < highPos) {
            // if(currentDistance < 100 && currentDistance > 5 ){
            if (currentDistance < highDistance && currentDistance > lowDistance ) {
              if (pos > 90) {
                pos = 170;
              } else if (pos <= 90) {
                pos = 10;
              }

              distancePreviousMillis = millis();
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
            //        Serial.print("Pos: ");
            //        Serial.println(pos);
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

        // Testing commented out oct 30th evening
//        Serial.println();
//        Serial.print("ID Pattern: ");
//        Serial.println(id);
//        Serial.println("===========");

        
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

//  NewPing(A4, A5, MAX_DISTANCE),
//  NewPing(A6, A7, MAX_DISTANCE),
//  NewPing(A8, A9, MAX_DISTANCE),
//  NewPing(A10, A11, MAX_DISTANCE),
//  NewPing(A12, A13, MAX_DISTANCE),
//  NewPing(A14, A15, MAX_DISTANCE),
//  NewPing(38, 39, MAX_DISTANCE),
//  NewPing(40, 41, MAX_DISTANCE)
};


//#define OBJECT_NUM  5 // Number of sensors.
//#define OBJECT_NUM_V2  8 // Number of sensors.
//#define OBJECT_NUM_TOTAL  13 // Number of sensors.

#define OBJECT_NUM  5 // Number of sensors.

// Sensor object array.
// ID, Update Interval, Sonar ID, Start Possition, mode
Sweeper sweep[OBJECT_NUM] = {
  Sweeper(0, 20, sonar[0], 0, mode),
  Sweeper(1, 20, sonar[1], 0, mode),
  Sweeper(2, 20, sonar[2], 0, mode),
  Sweeper(3, 20, sonar[3], 0, mode),
  Sweeper(4, 20, sonar[4], 0, mode)
  
//  Sweeper(5, 20, sonar[5], 0, mode),
//  Sweeper(6, 20, sonar[6], 0, mode),
//  Sweeper(7, 20, sonar[7], 0, mode),
//  Sweeper(8, 20, sonar[8], 0, mode),
//  Sweeper(9, 20, sonar[9], 0, mode),
//  Sweeper(10, 20, sonar[10], 0, mode),
//  Sweeper(11, 20, sonar[11], 0, mode),
//  Sweeper(12, 20, sonar[12], 0, mode)
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
//  Serial.begin(9600);
   Serial.begin(115200);

//   StaticJsonBuffer<1400> *jsonBuffer;

  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

//  StaticJsonBuffer<100> jsonBuffer;
//  JsonObject& root = jsonBuffer.createObject();


  // First ping starts at 75ms, gives time for the Arduino to chill before starting.
  pingTimer[0] = millis() + 75;
  // Set the starting time for each sensor.
  for (uint8_t i = 1; i < SONAR_NUM; i++) {
    pingTimer[i] = pingTimer[i - 1] + PING_INTERVAL;
  }

  int pin = 22;
  // Set the starting time for each sensor.
  for (uint8_t i = 0; i < OBJECT_NUM; i++) {
    sweep[i].Attach(pin);
    pin++;
  }

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
//    Serial.write("+=+=+=");
//    Serial.write(0);
  }

//  Serial.write(1);
//  Serial.print("qwerqwerqwerqwer");
//  Serial.write("sasdfasdfasdfasdf");
//  Serial.println("zxcvzxcvzxcv");
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
      Serial.println(i);
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

  // stop the loop if it is anything other than 'g' (go)
//  if (incomingByte != 103) {
//    return;
//  }

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

  if (sonar[currentSensor].check_timer()) {
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

    // commented out oct 30 eve
//    Serial.print(i);
//    Serial.print("=");
//    Serial.print(cm[i]);
//    Serial.print("cm ");

    
  }
//  Serial.println();
}

