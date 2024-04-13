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

int RF_CE = 9;
int RF_CSN = 10;

RF24 radio(RF_CE, RF_CSN);  // using pin 7 for the CE pin, and pin 8 for the CSN pin.  9 and 10 for joystick board

uint8_t address[][16] = { "1Node", "2Node", "3Node", "4Node", "5Node", "6Node", "7Node", "8Node", "9Node", "10Node", "11Node", "12Node", "13Node", "14Node", "15Node", "16Node" };
int payload[8];                                     // array to hold received data.  See transmitter code to view which array elements contain analog and digitial button data.
int ackData[8] = { 0, 0, -1, -1, -1, -1, -1, -1 };  // the two values to send back to the remote, just using random numbers to test; "404" is to show an error

int BUZZER = 4;
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
long pulseDuration;
int boost = 1;
int retry = 0;  // used for disabling motors if robot is disconnected for long enough
int Svo1pos = 0;
int Svo2pos = 0;
int deadzone = 10;  // set deadzone to 10
//some boards need to wait to ensure access to serial over USB

void setup() {
  printf_begin();  // needed only once for printing details
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
  //distances();
  sendAckData();
}
void getData() {
  uint8_t pipe;
  if (radio.available(&pipe)) {              // is there a payload? get the pipe number that recieved it
    uint8_t bytes = radio.getPayloadSize();  // get the size of the payload
    radio.read(&payload, bytes);             // fetch payload from FIFO
    Serial.print(F("Received "));
    Serial.print(bytes);  // print the size of the payload
    Serial.print(F(" bytes on pipe "));
    Serial.print(pipe);  // print the pipe number
    Serial.print(F(": "));
    for (int x = 0; x <= 6; x++) {  // print the payload's value
      Serial.print(payload[x]);
      Serial.print(F(" "));
    }
    Serial.println(payload[7]);
    retry = 0;
  } else {  // if communication is lost then set all values to default
    retry++;
    if (retry > 10) {
      payload[0] = 0;
      payload[1] = 0;
      for (int x = 2; x < 8; x++) {
        payload[x] = 1;
      }
      for (int x = 0; x < 3; x++) {  //beep 3 times quickly so user knows communication was lost
        digitalWrite(BUZZER, HIGH);
        delay(100);
        digitalWrite(BUZZER, LOW);
        delay(100);
      }
      resetFunc();  //reset the arduino so maybe it will regain communication
    }
    delay(50);
  }
}
void controlRobot() {
  if (payload[2] == 0) {                       // ButtonA is pressed, engage servo control
    Svo1pos = (payload[0] + 100) * 180 / 200;  // Convert X(-100 to 100) to int from 0-180
    if (payload[0] >= 99) Svo1pos = 180;       // fix for weird behaviour
    Svo2pos = (payload[1] + 100) * 180 / 200;  // Convert Y(-100 to 100) to int from 0-180
    if (payload[1] >= 99) Svo2pos = 180;       // fix for weird behaviour
    Servo1.write(Svo1pos);
    Servo2.write(Svo2pos);
  } else {  // ButtonA isn't pressed, control motors instead
    if (!(abs(payload[0]) <= deadzone && abs(payload[1]) <= deadzone)) {
      S = payload[0];
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
      Serial.println(M1speed);
      Serial.println("M2Speed: ");
      Serial.println(M2speed);
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
  ackData[0] = M1speed;
  ackData[1] = M2speed;
  ackData[5] = Servo1.read();
  ackData[6] = Servo2.read();
  Serial.println("Writing ackData");
  radio.writeAckPayload(RFpipe, &ackData, sizeof(ackData));  // load the payload for the next time
}
void distances() {  // Calculate distances from distance sensors and put into ackData; yes, I know a for loop would be better, but I haven't figured out how to that with 3 different variables
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
