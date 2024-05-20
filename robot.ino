/* 
This code is for an Arduino Uno reciever with motor control board and nRF24 transceiver.

Note the "ChannelFrequency", "RFpipe", and "gfxInterval" variables.  Change this so your transmitter and receiver use the same number so they are paired together and waiting the same amount of time.
Note the "gfxInterval" variable, change this to adjust input delay !!!Numbers too low will cause constant errors!!!
Note that the serial monitor should be set to 115200baud to monitor the Serial.print() output. 

You need to install the library titled "RF24" from the library manager.  "RF24" by "TMRh20,Avamander"
*/

#include <SPI.h>
#include <printf.h>
#include <RF24.h>
#include <Servo.h>

// ---THESE VARIABLES MUST MATCH ON TRANSMITTER AND ROBOT---
byte const ChannelFrequency = 24;  // !!! Frequency used by transmitter = 2,400mhz + ChannelFrequency.  Must be between 0 and 124 to work.  MUst be between 0 and 83 to stay legal.  Must match on both transceivers.
byte const RFpipe = 0;             // !!! This is the pipe used to receive data.  Choose a number between 0 and 15.  Must match on both transceivers.
int const gfxInterval = 30;  // interval to wait for graphics update, VERY SENSITIVE. Affects input delay greatly
int const llTime = 5; // set Low Latency Mode wait to 5ms
// ---End matching vars---

int scoopPos = 0; // the current position of the scoop up/down (uses the big servos attached to the robot)
int scoopRot = 0; // the current positino of the scoop rotation (uses the small servos attached to the arms)
int const Svo1Start = 0;
int const Svo1End = 90;
int const Svo2Start = 0; 
int const Svo2End = 0;
int const Svo3Start = 0;
int const Svo3End = 180;
int const Svo4Start = 180;
int const Svo4End = 0;

int const RF_CE = 9; // Pin for enabling RF24
int const RF_CSN = 10; // Pin for selecting Tx/Rx on RF24
RF24 radio(RF_CE, RF_CSN);  // using pin 7 for the CE pin, and pin 8 for the CSN pin.  9 and 10 for joystick board

uint8_t const address[][16] = { "1Node", "2Node", "3Node", "4Node", "5Node", "6Node", "7Node", "8Node", "9Node", "10Node", "11Node", "12Node", "13Node", "14Node", "15Node", "16Node" };
int payload[8] = {0, 0, 1, 1, 1, 1, 1, 0}; // array for holding recieved data, these are default values so the motors/servos don't move until we recieve a real transmission
int ackData[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };  // the two values to send back to the remote, just using 0's for example
int msg, code, amount; // vars for Serial input

int const BUZZER = 4; // Active Buzzer on Pin4, does not require tone() command
bool buzzerEnabled = false; // Set to false if you think it's too loud
// motor control io pin assignments
int const M1pwmPin = 5;  //IOpin assignment, enable for motor1 (pwm on this pin). IO pin10 is default, but changed to leave SPI port available
int const M1dirPin = 3;  //IOpin assignment, direction for motor1.  IO pin12 is default

int const M2pwmPin = 6;  //IOpin assignment, enable for motor2 (pwm on this pin). IO pin11 is default, but changed to leave SPI port available
int const M2dirPin = 2;  //IOpin assignment, direction for motor2. IO pin13 is default

int M1speed = 0;  // variable holding speed of motor, value of 0-100
int M2speed = 0;  // variable holding speed of motor, value of 0-100

bool M1dir = 1;
bool M2dir = 1;
bool doingLL = false; // controls Low Latency Mode

float S;
float T;
float maximum;
float total;
float diff;

// Hobby Servo io pin assignments
int const Svo1pin = A0;
int const Svo2pin = A1;
int const Svo3pin = 18; // pin A4
int const Svo4pin = 19; // pin A5
Servo Servo1;  // create instance of servo
Servo Servo2;  // create instance of servo
Servo Servo3;
Servo Servo4;
//---Distance Sensor Vars---
int const trigger = 7;  // specify pin used to trigger distance sensors, connect this to all of them
int const x1Pin = -1;   // Pin for leftmost distance sensor echo
int const x2Pin = 8;    // Pin for middle distance sensor echo
int const x3Pin = -1;   // Pin for rightmost distance sensor echo
long unsigned int dstTime;
bool dstEnabled = false;
long pulseDuration;
int retry = 0;  // used for disabling motors if robot is disconnected for long enough

int const deadzone = 5;  // set deadzone to 5

void (*resetFunc)(void) = 0;  // declare reset function at address 0

void setup() {
  printf_begin();  // needed only once for printing details
  pinMode(trigger, OUTPUT);
  pinMode(x2Pin, INPUT);
  Serial.begin(115200);
  while (!Serial) {
    // some boards need to wait to ensure access to serial over USB
  }
  Serial.println(F("-------Welcome to the Robot Serial------"));
  if (!radio.begin()) {
    Serial.println(F("radio hardware is not responding!"));
    delay(1000);
    resetFunc(); // reset the Arduino
  }
  Servo1.attach(Svo1pin); // attach Servo1 to Svo1pin
  Servo2.attach(Svo2pin);
  Servo3.attach(Svo3pin);
  Servo3.write(Svo3Start);
  Servo4.attach(Svo4pin);
  Servo4.write(Svo3Start); // set Servo4 to starting position
// ---RF24 Initialization---
  radio.setDataRate(RF24_250KBPS);
  radio.enableAckPayload();  // enables sending data back to transmitter
  radio.setChannel(ChannelFrequency);                        // sets the frequency between 2,400mhz and 2,524mhz in 1mhz incriments
  radio.setPALevel(RF24_PA_MAX);                             // RF24_PA_MAX is default.
  radio.setPayloadSize(sizeof(payload));                     // set the payload size, must be < 32bytes
  radio.openReadingPipe(RFpipe, address[RFpipe]);            // open the pipe for reading from the radio
  radio.startListening();                                    // put radio in RX mode
  radio.writeAckPayload(RFpipe, &ackData, sizeof(ackData));  // pre-load data
  pinMode(BUZZER, OUTPUT);                                   // set buzzer pin to output
} // end of void setup()
void processInput() {
  msg = Serial.parseInt();
  code = msg/1000; // get the first digit
  amount = msg - (code*1000); // get the last three digits
  Serial.println(F("Control Code: "));
  Serial.println(amount);
  switch (code) {
    case 1:
      Serial.println(F("Controlling Motors"));
      payload[0] = 100;
      payload[1] = 100;
      controlRobot();
    case 2:
      Serial.println(F("Controlling Scoop Arms"));
      if (amount <= 180 && amount >= 0) { // check that "amount" is within limits
        Servo1.write(amount);
        Servo2.write(map(amount, 0, 180, 180, 0)); // reverse "amount"
      }
    case 3:
      Serial.println(F("Controlling Scoop"));
      if (amount <= 180 && amount >= 0) {
        Servo3.write(map(amount, 0, 180, Svo3Start, Svo3End));
        Servo4.write(map(amount, 0, 180, Svo4Start, Svo4End)); // reverse "amount"
      }
  }
}

void loop() {
  getData();
  if (Serial.available()) processInput(); // if there is a serial message available, process the input
  else controlRobot();
  sendAckData();
  //distances(); decided to remove dst sensors, as the scoop will get in the way
  if (doingLL) delay(llTime); // if using Low Latency mode, delay by llTIme
  else delay(gfxInterval); // otherwise,delay by gfxInterval
} // end of void loop()

void getData() {
  uint8_t pipe;
  if (radio.available(&pipe)) {              // is there a payload? get the pipe number that recieved it
    uint8_t bytes = radio.getPayloadSize();  // get the size of the payload
    radio.read(&payload, bytes);             // fetch payload from FIFO(File In File Out)
    if (payload[7] == 1) {doingLL = true; ackData[7] = 1;} // if payload[7] is equal to 1, engage Low Latency Mode and update ackData to show Txer it has been recieved
    else doingLL = false; ackData[7] = 0;// ^ if it isn't make doingLL false and update ackData as well
    Serial.print(F("Received "));
    Serial.print(bytes);  // print the size of the payload
    Serial.print(F(" bytes on pipe "));
    Serial.print(pipe);  // print the pipe number
    Serial.print(F(": "));
    for (int x = 0; x <= 6; x++) {  // print the payload's value
      Serial.print(payload[x]);
      Serial.print(F(" "));
    }
    retry = 0;
  } else {  // if communication is lost then set all values to default
    retry++;
    if (retry > 10) {
      payload[0] = 0;
      payload[1] = 0;
      for (int x = 2; x < 8; x++) { // make all buttons set to not pressed
        payload[x] = 1;
      }
      for (int x = 0; x < 3; x++) {  // beep 3 times quickly so user knows communication was lost
        if (buzzerEnabled) {
          digitalWrite(BUZZER, HIGH);
          delay(100);
          digitalWrite(BUZZER, LOW);
          delay(100);
        }
      }
      //resetFunc();  // reset the arduino so maybe it will regain communication
    }
  } // end of com lost
} // end of void getData()\

void controlRobot() {
  if (payload[7] == 0) dstEnabled = true; // if ButtonF is pressed, re-enable distance sensors
  if (buzzerEnabled) { // if the buzzer is enabled, check for horn button
    if (payload[3] == 0) digitalWrite(BUZZER, HIGH); // if ButtonB pressed and buzzer enabled, turn on horn
    else digitalWrite(BUZZER, LOW);
  }
  if (payload[4] == 0) { // button is pressed, engage scoop servo control
    Servo3.write(Svo3End);
    Servo4.write(Svo4End);
  } else { // button is not pressed, go to normal position
    //Servo3.write(Svo3Start);
    //Servo4.write(Svo4Start);
  }
  if (payload[2] == 0) {                  // ButtonA is pressed, engage servo control
    M1speed = 0; // make motor speed zero when controlling servos
    M2speed = 0; // make motor speed zero when controlling servos
    //---Calculate Values---
    scoopPos = map(payload[0], -100, 100, 0, 180);  // Convert X(-100 to 100) to int from 0-180
    scoopRot = map(payload[1], -100, 100, 0, 180); //  Convert Y(-100 to 100) to int from 0-180
    //---Write Values to Servos--- 
    Servo1.write(scoopPos); // write value to Servo 1
    Servo2.write(map(scoopPos, 0, 180, 180, 0)); // write value to Servo 2, using map() to reverse it
    Servo3.write(scoopRot); // write scoop rotation to Servo 3
    Servo4.write(map(scoopRot, 0, 180, 180, 0)); // write scoop rotation to Servo4, using map() to reverse it    
  } 
  else {  // ButtonA isn't pressed, control motors instead
    if (!(abs(payload[0]) <= deadzone && abs(payload[1]) <= deadzone)) { // if the joystick is outside of the deadzones, run the motor control
      if (payload[6] == 1) S = payload[0] * 0.75; // if joyButton not pressed make turning speed only 3/4 of full, to make it more easy to control
      else S = payload[0]; // if it is pressed, don't adjust the turning speed
      T = payload[1]; // throttle = joystickY
      //---Arcade Drive Math, adapted from <https://xiaoxiae.github.io/Robotics-Simplified-Website/drivetrain-control/arcade-drive/>---
      maximum = max(abs(T), abs(S));
      total = T + S;
      diff = T - S;
      if (T >= 0){
        if (S >= 0){ // I quadrant
          M1speed = maximum;
          M2speed = diff;
        }
        else { // II quadrant
          M1speed = total;
          M2speed = maximum;
        }
      }
      else {
        if (S >= 0){ // IV quadrant
          M1speed = total;
          M2speed = maximum * -1;
        }
        else { // III quadrant
          M1speed = maximum * -1;
          M2speed = diff;
        }
      }
      if (M1speed < 0){ // calculate direction based off of value of M1speed
        M1speed *= -1;
        M1dir = 1;
      }
      else M1dir = 0;
      if (M2speed < 0){ // calculate direction based off of value of M2speed
        M2speed *= -1;
        M2dir = 1;
      }
      else M2dir = 0;
      Serial.println("M1Speed: ");
      Serial.print(M1speed);
      Serial.println("M2Speed: ");
      Serial.print(M2speed);
      digitalWrite(M1dirPin, M1dir);
      digitalWrite(M2dirPin, M2dir);
      analogWrite(M1pwmPin, M1speed*2.54);
      analogWrite(M2pwmPin, M2speed*2.54);
    } else {  // set both motors to zero, if X&Y are within deadzone
      analogWrite(M1pwmPin, 0);
      analogWrite(M2pwmPin, 0);
      M1speed = 0;
      M2speed = 0;
    }
  } // end of controlling motors (not servos)
} // end of void controlRobot()

void sendAckData() {
  if (M1dir == 0) M1speed = M1speed * -1;  // make M1speed negative if the direction is backwards
  if (M2dir == 0) M2speed = M2speed * -1;  // make M2speed negative if the direction is backwards
  // sanitize ackData, because of floating point math errors
  /*if (M1speed > 100) M1speed = 100;
  if (M1speed < -100) M1speed = -100;
  if (M2speed > 100) M2speed = 100;
  if (M2speed < -100) M2speed = -100;
  if (Svo2pos > 180) Svo2pos = 180;
  if (Svo2pos < 0) Svo2pos = 0;*/
  ackData[0] = M1speed;
  ackData[1] = M2speed;
  //ackData[5] = scoopPos;
  ackData[5] = Servo1.read();
  //ackData[6] = scoopRot;
  ackData[6] = Servo3.read();
  Serial.println(F(""));
  Serial.print(F("Writing ackData["));
  radio.writeAckPayload(RFpipe, &ackData, sizeof(ackData));  // load the payload for the next time
  for (int i = 0; i <= 7; i++) { // print ackData to Serial
      Serial.print(ackData[i]);
      if (i != 7) Serial.print(F(",")); // if it is not the last index, print a comma
  }
  Serial.print(F("]"));
}

void distances() {  // Calculate distances from distance sensors and put into ackData; yes, I know a for loop would be better, but I haven't figured out how to that with 3 different variables
  dstTime = millis();
  if (dstEnabled){
    //digitalWrite(trigger, HIGH); // trigger the HC-SR04s
    //delayMicroseconds(10); // give enough time for the HC-SR04s to detect the trigger
    //digitalWrite(trigger, LOW); // un-trigger the HC-SR04s
    //pulseDuration = pulseIn(x1Pin, HIGH); // find time of pulse for x1
    //ackData[2] = pulseDuration * 0.0171; // find distance in cm for x1
    digitalWrite(trigger, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigger, LOW);
    pulseDuration = pulseIn(x2Pin, HIGH);  // find time of pulse for x2
    ackData[3] = pulseDuration * 0.0171;   // find distance in cm for x2
    //digitalWrite(trigger, HIGH);
    //delayMicroseconds(10);
    //digitalWrite(trigger, LOW);
    //pulseDuration = pulseIn(x3Pin, HIGH); // find time of pulse for x3
    //ackData[4] = pulseDuration * 0.0171; // find distance in cm for x3
  }
  dstTime = millis() - dstTime;
  //ackData[7] = dstTime; // no longer needed, no more distance sensors
  if (doingLL) delay(llTime); // if in Low Latency Mode, wait llTIme
  else {
    if (dstTime > gfxInterval) {
    dstEnabled = false; // if the time is greater than gfxInterval ms, disable distance sensors
    Serial.print(F("DISTANCE SENSORS DISABLED"));
    }
    else { // if the dstTime is normal, wait gfxInterval ms accounting for the time the distance sensor takes
      if (dstEnabled) delay(gfxInterval - dstTime);
      else delay(gfxInterval - 5);
    }
  }
  Serial.print(F("dstTime: "));
  Serial.println(dstTime);
}