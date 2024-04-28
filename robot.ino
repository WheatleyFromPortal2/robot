/* 
This code is for an Arduino Uno transmitter with joystick control board and nRF24 transceiver.
Note the ChannelFrequency and RFpipe variables.  Change this so your transmitter and receiver use the same number so they are paired together.
Note that the serial monitor should be set to 115200bps to monitor the serial.print() output. 
You need to install the library titled "RF24" from the library manager.  "RF24" by "TMRh20,Avamander"
The RF24 related code is based off reference code found here: https://github.com/nRF24/RF24/blob/master/examples/GettingStarted/GettingStarted.ino

v2.2 adds resetting if communications is lost
v2.3 fixed studdering bug caused by communications restting routine
*/

#include <SPI.h>
#include <printf.h>
#include <RF24.h>
#include <Servo.h>

// ---RF24 settings and IO pin assignments---
byte ChannelFrequency = 24;  //!!!!!!!!!!!!!!  Frequency used by transmitter = 2,400mhz + ChannelFrequency.  Must be between 0 and 124 to work.  MUst be between 0 and 83 to stay legal.  Must match on both transceivers.
byte RFpipe = 0;             //!!!!!!!!!!!!!!  This is the pipe used to receive data.  Choose a number between 0 and 15.  Must match on both transceivers.

int const RF_CE = 9;
int const RF_CSN = 10;
int const gfxInterval = 25; // interval to wait for graphics update, VERY SENSITIVE. Affects input delay greatly
// Svo2 needs to be reversed
int const Svo1Start = 0;
int const Svo1End = 90;
int const Svo2Start = 90; // purposely reversed, as Servo2 is mounted going the opposite direction
int const Svo2End = 0;    // purposely reversed ^
RF24 radio(RF_CE, RF_CSN);  // using pin 7 for the CE pin, and pin 8 for the CSN pin.  9 and 10 for joystick board

uint8_t address[][16] = { "1Node", "2Node", "3Node", "4Node", "5Node", "6Node", "7Node", "8Node", "9Node", "10Node", "11Node", "12Node", "13Node", "14Node", "15Node", "16Node" };
int payload[8];                                     // array to hold received data.  See transmitter code to view which array elements contain analog and digitial button data.
int ackData[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };  // the two values to send back to the remote, just using 0's for example

int BUZZER = 0; // disable buzzer
// motor control io pin assignments
int M1pwmPin = 5;  //IOpin assignment, enable for motor1 (pwm on this pin). IO pin10 is default, but changed to leave SPI port available
int M1dirPin = 3;  //IOpin assignment, direction for motor1.  IO pin12 is default

int M2pwmPin = 6;  //IOpin assignment, enable for motor2 (pwm on this pin). IO pin11 is default, but changed to leave SPI port available
int M2dirPin = 2;  //IOpin assignment, direction for motor2. IO pin13 is default

int M1speed = 0;  // variable holding speed of motor, value of 0-100
int M2speed = 0;  // variable holding speed of motor, value of 0-100

bool M1dir = 1;
bool M2dir = 1;

float S;
float T;
float maximum;
float total;
float diff;

// Hobby Servo io pin assignments
int Svo1pin = A0;
int Svo2pin = A1;
Servo Servo1;  // create instance of servo
Servo Servo2;  // create instance of servo

//---Distance Sensor Vars---
int const trigger = 7;  // specify pin used to trigger distance sensors, connect this to all of them
int const x1Pin = -1;   // Pin for leftmost distance sensor echo
int const x2Pin = 8;    // Pin for middle distance sensor echo
int const x3Pin = -1;   // Pin for rightmost distance sensor echo
long unsigned int dstTime;
bool dstEnabled = false;
long pulseDuration;
int boost = 1;
int retry = 0;  // used for disabling motors if robot is disconnected for long enough
int Svo1pos = 0;
int Svo2pos = 0;
int deadzone = 10;  // set deadzone to 10
//some boards need to wait to ensure access to serial over USB

void setup() {
  printf_begin();  // needed only once for printing details
  pinMode(trigger, OUTPUT);
  pinMode(x2Pin, INPUT);
  Serial.begin(115200);
  while (!Serial) {
    // some boards need to wait to ensure access to serial over USB
  }
  if (!radio.begin()) {
    Serial.println(F("radio hardware is not responding!!"));
    while (1) {}  // hold in infinite loop
  }
  Servo1.attach(Svo1pin);
  Servo2.attach(Svo2pin);
  radio.setDataRate(RF24_250KBPS);

  radio.enableAckPayload();  // enables sending data back to transmitter

  radio.setChannel(ChannelFrequency);                        // sets the frequency between 2,400mhz and 2,524mhz in 1mhz incriments
  radio.setPALevel(RF24_PA_MAX);                             // RF24_PA_MAX is default.
  radio.setPayloadSize(sizeof(payload));                     // set the payload size, must be < 32bytes
  radio.openReadingPipe(RFpipe, address[RFpipe]);            // open the pipe for reading from the radio
  radio.startListening();                                    // put radio in RX mode
  radio.writeAckPayload(RFpipe, &ackData, sizeof(ackData));  // pre-load data
  pinMode(BUZZER, OUTPUT);                                   // set buzzer pin to output

  payload[0] = 0;  // this code puts in default values for the payload
  payload[1] = 0;
  for (int x = 2; x < 8; x++) {
    payload[x] = 1;
  }
}

void (*resetFunc)(void) = 0;  //declare reset function at address 0


void loop() {
  getData();
  controlRobot();
  sendAckData();
  //distances(); decided to remove dst sensor, as the scoop will get in the way
  delay(gfxInterval);
}
void getData() {
  uint8_t pipe;
  if (radio.available(&pipe)) {              // is there a payload? get the pipe number that recieved it
    uint8_t bytes = radio.getPayloadSize();  // get the size of the payload
    radio.read(&payload, bytes);             // fetch payload from FIFO(File In File Out)
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
      for (int x = 2; x < 8; x++) {
        payload[x] = 1;
      }
      for (int x = 0; x < 3; x++) {  // beep 3 times quickly so user knows communication was lost
        digitalWrite(BUZZER, HIGH);
        delay(100);
        digitalWrite(BUZZER, LOW);
        delay(100);
      }
      resetFunc();  // reset the arduino so maybe it will regain communication
    }
  }
}
void controlRobot() {
  if (payload[7] == 0) dstEnabled = true; // if ButtonF is pressed, re-enable distance sensors
  if (payload[2] == 0) {                  // ButtonA is pressed, engage servo control
    if (payload[5] == 0) { // if ButtonD is pressed, do old method
      // ---Old Servo control method---
      Svo1pos = (payload[0] + 100) * 180 / 200;  // Convert X(-100 to 100) to int from 0-180
      if (payload[0] >= 99) Svo1pos = 180;       // fix for weird behaviour
      Svo2pos = (payload[1] + 100) * 180 / 200;  // Convert Y(-100 to 100) to int from 0-180
      if (payload[1] >= 99) Svo2pos = 180;       // fix for weird behaviour // ButtonA is pressed, engage servo control
    } else { // ---New Servo Control Method, for Scoop--- activated if ButtonD is not pressed
      Svo1pos = map(payload[0], -100, 100, Svo1Start, Svo1End); // map the X(-100 to 100) to the Servo 1 Start and Servo1 End values
      Svo2pos = map(payload[1], -100, 100, Svo2Start, Svo2End); // map the Y(-100 to 100) to the Servo 2 Start and Servo2 End values
    }
    Servo1.write(Svo1pos); // write value to Servo 1
    Servo2.write(Svo2pos); // write value to Servo 2
    
  } else {  // ButtonA isn't pressed, control motors instead
    if (!(abs(payload[0]) <= deadzone && abs(payload[1]) <= deadzone)) {
      S = payload[0] * 0.75; // make turning speed only 3/4 of full, to make it more easy to control
      T = payload[1];
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
      if (M1speed < 0){
        M1speed *= -1;
        M1dir = 1;
      }
      else M1dir = 0;
      if (M2speed < 0){
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
  }
}
void sendAckData() {
  if (M1dir == 0) M1speed = M1speed * -1;  // make M1speed negative if the direction is backwards
  if (M2dir == 0) M2speed = M2speed * -1;  // make M2speed negative if the direction is backwards
  // sanitize ackData, because of floating point math errors
  if (M1speed > 100) M1speed = 100;
  if (M1speed < -100) M1speed = -100;
  if (M2speed > 100) M2speed = 100;
  if (M2speed < -100) M2speed = -100;
  Svo1pos = Servo1.read();
  Svo2pos = Servo2.read();
  if (Svo1pos > 180) Svo1pos = 180;
  if (Svo1pos < 0) Svo1pos = 0;
  if (Svo2pos > 180) Svo2pos = 180;
  if (Svo2pos < 0) Svo2pos = 0;
  ackData[0] = M1speed;
  ackData[1] = M2speed;
  ackData[5] = Svo1pos;
  ackData[6] = Svo2pos;
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
  ackData[7] = dstTime;
  if (dstTime > gfxInterval) {
    dstEnabled = false; // if the time is greater than gfxInterval ms, disable distance sensors
    Serial.print("DISTANCE SENSORS DISABLED");
  }
  else { // if the dstTime is normal, wait gfxInterval ms accounting for the time the distance sensor takes
    if (dstEnabled) delay(gfxInterval - dstTime);
    else delay(gfxInterval - 5);
  }
  Serial.print("dstTime: ");
  Serial.println(dstTime);
}
