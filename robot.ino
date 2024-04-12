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
int payload[8];  // array to hold received data.  See transmitter code to view which array elements contain analog and digitial button data.
int ackData[8] = {0, 0,-1 , -1, -1, -1, -1, -1}; // the two values to send back to the remote, just using random numbers to test; "404" is to show an error
  
bool goodSignal;
int BUZZER = 4;
// motor control io pin assignments
int M1pwmPin = 5;  //IOpin assignment, enable for motor1 (pwm on this pin). IO pin10 is default, but changed to leave SPI port available
int M1dirPin = 3;  //IOpin assignment, direction for motor1.  IO pin12 is default

int M2pwmPin = 6;  //IOpin assignment, enable for motor2 (pwm on this pin). IO pin11 is default, but changed to leave SPI port available
int M2dirPin = 2;  //IOpin assignment, direction for motor2. IO pin13 is default

int M1speed = 255;  // variable holding PWM (Pulse Width Modulation) speed of motor, value of 0-255
int M2speed = 255;  // variable holding PWM (Pulse Width Modulation) speed of motor, value of 0-255

bool M1dir=1;
bool M2dir=1;

// Hobby Servo io pin assignments
int Svo1pin = 14;
int Svo2pin=15; 
Servo Servo1;  // create instance of servo
Servo Servo2; // create instance of servo

//---Distance Sensor Vars---
int const trigger = 8; // specify pin used to trigger distance sensors, connect this to all of them
int const x1Pin = A0; // Pin for leftmost distance sensor echo
int const x2Pin = A1; // Pin for middle distance sensor echo
int const x3Pin = A2; // Pin for rightmost distance sensor echo
long pulseDuration;

int retry = 0; // used for disabling motors if robot is disconnected for long enough

bool Svo1On = false;
bool Svo2On = false;
int deadzone = 10; // set deadzone to 10
//some boards need to wait to ensure access to serial over USB
void setup(){
  printf_begin();             // needed only once for printing details
  if (!radio.begin()) {
    Serial.println(F("radio hardware is not responding!!"));
    while (1) {}  // hold in infinite loop
  }
  radio.setDataRate(RF24_250KBPS); 

  radio.enableAckPayload(); // enables sending data back to transmitter

  radio.setChannel(ChannelFrequency);  // sets the frequency between 2,400mhz and 2,524mhz in 1mhz incriments
  radio.setPALevel(RF24_PA_MAX);       // RF24_PA_MAX is default.
  radio.setPayloadSize(sizeof(payload)); // set the payload size, must be < 32bytes
  radio.openReadingPipe(RFpipe, address[RFpipe]); // open the pipe for reading from the radio
  radio.startListening();                               // put radio in RX mode
  radio.writeAckPayload(RFpipe, &ackData, sizeof(ackData));  // pre-load data
  pinMode(BUZZER, OUTPUT); // set buzzer pin to output

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
  if (!(abs(payload[0]) <= deadzone && abs(payload[1]) <= deadzone)){
    if (payload[0]>50 || payload[0] < -50 ){ // if the robot needs to turn
      if (payload[0]<50){
        M1speed = (payload[0]-90)*7/5+30; 
        M2speed = (payload[0]-75)*7/5+30; 
        M1dir= 1; 
        M2dir= 0;
      }
      else{
        M1speed = (payload[0]*-1-90)*7/5+30; 
        M2speed = (payload[0]*-1-90)*7/5+30; 
        M1dir= 0; 
        M2dir= 1;
      }
      
    }
    if (payload[1]>50){
        M1speed = (payload[1]-50)*7/5+30; 
        M2speed = (payload[1]-50)*7/5+30; 
        M1dir= 1; 
        M2dir= 1;
    }
    if (payload[1]<-50){
        M1speed = (payload[1]*-1-50)*7/5+30; 
        M2speed = (payload[1]*-1-50)*7/5+30; 
        M1dir= 0; 
        M2dir= 0; 
    }
    if (payload[4]){
      M1speed= 100; 
      M2speed= 100;
    }
    digitalWrite(M1dirPin, M1dir);
    digitalWrite(M2dirPin, M2dir);
    analogWrite(M1pwmPin, M1speed*2.54);
    analogWrite(M2pwmPin, M2speed*2.54);
  }
  else { // set both motors to zero, if X&Y are within deadzone
    analogWrite(M1pwmPin, 0);
    analogWrite(M2pwmPin, 0);
  }


  // These last lines show how to make hobby servo go to a position when button press is received
  //---Servo Control---
  if (payload[2]==0 && !Svo1On){ // buttonA pressed, and Servo1 is not extended
    Servo1.write(180); // extend Servo1
    Svo1On = true; // Servo1 is now extended
  }
  if (payload[2]==1 && Svo1On){ // buttonA not pressed, and Servo1 extended
    Servo1.write(10); // unextend Servo1
    Svo1On = false; // Servo1 is no longer extended
  }
  if (payload[3]==0 && !Svo2On){ // buttonB pressed, and Servo2 is not extended
    Servo2.write(180); // extend Servo2
    Svo2On = true; // Servo2 is now extended
  }
  if (payload[3]==1 && Svo2On){ // buttonB not pressed, and Servo2 extended
    Servo2.write(10); // unextend Servo2
    Svo2On = false; // Servo2 is no longer extended
  }
  if (payload[3]==0) digitalWrite(BUZZER, HIGH);  //turn on buzzer
      else digitalWrite(BUZZER, LOW); // turn off buzzer
}
void sendAckData(){
  ackData[0] = M1speed; 
  ackData[1]= M2speed; 
  ackData[5]= Servo1.read(); 
  ackData[6]= Servo2.read(); 
  bool goodSignal = radio.testCarrier();
  if (goodSignal == true) { // Check the signal strength and write "1" if it si good, and "0" if its bad to ackData[0]
    ackData[7] = 1;
    Serial.println("Strong signal > -64dBm");
  }
  else {
    ackData[7] = 0;
    Serial.println("Weak signal < -64dBm");
  }
  Serial.println("Writing ackData");
  radio.writeAckPayload(RFpipe, &ackData, sizeof(ackData)); // load the payload for the next time
}
void distances(){ // Calculate distances from distance sensors and put into ackData; yes, I know a for loop would be better, but I haven't figured out how to that with 3 different variables
  digitalWrite(trigger, HIGH); // trigger the HC-SR04s
  delayMicroseconds(10); // give enough time for the HC-SR04s to detect the trigger
  digitalWrite(trigger, LOW); // un-trigger the HC-SR04s
  pulseDuration = pulseIn(x1Pin, HIGH); // find time of pulse for x1
  ackData[2] = pulseDuration * 0.0171; // find distance in cm for x1
  digitalWrite(trigger, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigger, LOW);
  pulseDuration = pulseIn(x2Pin, HIGH); // find time of pulse for x2
  ackData[3] = pulseDuration * 0.0171; // find distance in cm for x2
  digitalWrite(trigger, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigger, LOW);
  pulseDuration = pulseIn(x3Pin, HIGH); // find time of pulse for x3
  ackData[4] = pulseDuration * 0.0171; // find distance in cm for x3
}
