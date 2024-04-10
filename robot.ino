#include <SPI.h>
#include <printf.h>
#include <RF24.h>
#include <Servo.h>
// William Test
//---NR24 Vars---
byte ChannelFrequency = 24; //!!!!!!!!!!!!!!  Frequency used by transmitter = 2,400mhz + ChannelFrequency.  Must be between 0 and 124 to work.  MUst be between 0 and 83 to stay legal.  Must match on both transceivers.
byte RFpipe=0;            //!!!!!!!!!!!!!!  This is the pipe used to receive data.  Choose a number between 0 and 15.  Must match on both transceivers.
int const RF_CE=9;
int const RF_CSN = 10;
RF24 radio(RF_CE, RF_CSN ); // using pin 7 for the CE pin, and pin 8 for the CSN pin.  9 and 10 for joystick board
uint8_t address[][16] = {"1Node", "2Node", "3Node", "4Node", "5Node", "6Node", "7Node", "8Node", "9Node", "10Node", "11Node", "12Node", "13Node", "14Node", "15Node", "16Node"};
const byte thisSlaveAddress[5] = {'R','x','A','A','A'};
int payload[8];  // array to hold received data.  See README.md to see what it holds
int ackData[8] = {109, -4000, -1, -1, -1, -1, -1, -1}; // the two values to send back to the remote, just using random numbers to test
bool newData = false;
//---Distance Sensor Vars---
int const trigger = 8; // specify pin used to trigger distance sensors, connect this to all of them
int const x1Pin = A0; // Pin for leftmost distance sensor echo
int const x2Pin = A1; // Pin for middle distance sensor echo
int const x3Pin = A2; // Pin for rightmost distance sensor echo
long pulseDuration;
//---L298P Motor Shield---
//  Motor A4
int const BUZZER = 4;
int const ENA = 5;  
int const INA = 3;
//  Motor B
int const ENB = 6;  
int const INB = 2;
int const MIN_SPEED = 10;   // Set to minimum PWM value that will make motors turn
int const ACCEL_DELAY = 50; // delay between steps when ramping motor speed up or down.
int mSpeed;
int retry=0;
//---Servo Vars---
int const Svo1pin = A0; // (A0 = pin14)
int const Svo2pin = A1;
Servo Servo1; // create instance of servo
Servo Servo2; // create instance of servo

void setup() {
  // L298P Motor Shield Portion
  pinMode(ENA, OUTPUT);   // set all the motor control pins to outputs
  pinMode(ENB, OUTPUT);
  pinMode(INA, OUTPUT);
  pinMode(INB, OUTPUT);
  pinMode(BUZZER, OUTPUT); // set buzzer pin to output
  // End 298P Portion
  pinMode(trigger, OUTPUT); // Set trigger pin for distance sensors to OUTPUT
  pinMode(x1Pin, INPUT); // Set echo pin for leftmost to INPUT
  pinMode(x2Pin, INPUT); // Set echo pin for middle to INPUT
  pinMode(x3Pin, INPUT); // Set echo pin for rightmost to INPUT
  Servo1.attach(Svo1pin); // Attach Servo1
  Servo2.attach(Svo2pin); // Attach Servo2
  Serial.begin(115200); // Begin Serial at 115200 Baud, !!!make sure the it is set to 115200 in Ardiuno IDE!!!
  while (!Serial) {
    // some boards need to wait to ensure access to serial over USB
  }
  if (!radio.begin()) {
    Serial.println(F("radio hardware is not responding!!"));
    while (1) {} // hold in infinite loop
  }
  radio.setDataRate(RF24_250KBPS);
  radio.setChannel(ChannelFrequency);  // sets the frequency between 2,400mhz and 2,524mhz in 1mhz incriments
  radio.setPALevel(RF24_PA_MAX);  // RF24_PA_MAX is default.
  radio.setPayloadSize(sizeof(payload));  
  radio.openReadingPipe(1, thisSlaveAddress);
  radio.enableAckPayload(); // Enables sending data back to transmitter
  radio.startListening(); // put radio in RX mode
  printf_begin();             // needed only once for printing details
  radio.printPrettyDetails(); // (larger) function that prints human readable data
  pinMode(4, OUTPUT);
  radio.writeAckPayload(RFpipe, &ackData, sizeof(ackData)); // pre-load data
}

void(* resetFunc) (void) = 0;//declare reset function at address 0

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

void loop() {
  
    uint8_t pipe;
    if (radio.available(&pipe)) {             // is there a payload? get the pipe number that recieved it
      uint8_t bytes = radio.getPayloadSize(); // get the size of the payload
      radio.read(&payload, bytes);            // fetch payload from FIFO
      Serial.print(F("Received "));
      Serial.print(bytes);                    // print the size of the payload
      Serial.print(F(" bytes on pipe "));
      Serial.print(pipe);                     // print the pipe number
      Serial.print(F(": "));
      for (int x=0; x<=6; x++){               // print the payload's value
        Serial.print(payload[x]);
        Serial.print(F(" "));
      }
      Serial.println(payload[7]);        
      retry=0;      
    }
    else{                                      // if communication is lost then set all values to default
      retry++;
      if (retry>10){
        payload[0]=0; // set motor values to zero, so it doesn't start moving on its own 
        payload[1]=0; 
        for(int x=2; x<8; x++){ // set all other values to 1, so the buttons aren't recognized as pressed
          payload[x]=1;
        }
        for (int x=0; x<3; x++){               //beep 3 times quickly so user knows communication was lost
          digitalWrite(4, HIGH);
          delay(100);
          digitalWrite(4, LOW);
          delay(100);
        }
        resetFunc();                           //reset the arduino so maybe it will regain communication
      }
      delay(50);
    }
        if (payload[4]){ // If "Button C" is pressed, ENGAGE AFTERBURNERS
          mSpeed = payload[1] / 2.0; // If the button isn't pressed, make is slower
          Serial.println("Normal Speed");
        }
        else {
          mSpeed = payload[1];
          Serial.println("ENGAGING AFTERBURNERS");
        }
        if (payload[1]>10){
         Motor('A', 'F', mSpeed); // Set Motor A to go forward and speed of payload
         Motor('B', 'R', mSpeed); // Set Motor B to go reverse and speed of payload
         
      }
      else if (payload[1]< -10){
         Motor('A', 'F', mSpeed); // Set Motor A to forward and speed of payload
         Motor('B', 'R', mSpeed); // Set Motor B to reverse and speed of payload
      }
      else if (payload[0]> 10){
         Motor('C', 'R', mSpeed); // Both motors go at mSpeed
      }
      else if (payload[0]< -10){
         Motor('C', 'F', mSpeed); // Both motors go at mSpeed
      }
      else {
         Motor('C', 'F', 0); // Set both motors to stop
     }


    // These last lines show how to make hobby servo go to a position when button press is received
    
    if (payload[2]==0) Servo1.write(10);  //send test hobby servo to position 10 when button A is pressed
    else Servo1.write(180);               //send test hobby servo to position 180 When button A is not pressed

    if (payload[3]==0) digitalWrite(4, HIGH);  //turn on buzzer
    else digitalWrite(4, LOW); // turn off buzzer
  // Find signal strength, maybe later assign to LED?
  bool goodSignal = radio.testRPD();
  if(radio.available()){
  Serial.println(goodSignal ? "Strong signal > -64dBm" : "Weak signal < -64dBm" );
  radio.read(&payload,sizeof(payload));
  }
  // ---Get ackData values from hardware, Motor data is in Motor function---
  
  ackData[5] = Servo1.read(); // '.read' function is misleading because it only reads the last written position, not the actual position, since that cannot be electrically done
  ackData[6] = Servo2.read(); // same issue here, but it's better than nothing

  if (goodSignal == true) { // Check the signal strength and write "1" if it si good, and "0" if its bad to ackData[0]
    ackData[7] = 1;
  }
  else {
    ackData[7] = 0;
  }
   Serial.println("Writing ackData");
   radio.writeAckPayload(RFpipe, &ackData, sizeof(ackData)); // load the payload for the next time
   distances(); // get fresh distance values, this may take a second, so we do it after sending the ackData
}

/*
 * Motor function does all the heavy lifting of controlling the motors
 * mot = motor to control either 'A' or 'B'.  'C' controls both motors.
 * dir = Direction either 'F'orward or 'R'everse
 * speed = Speed.  Takes in 1-100 percent and maps to 0-255 for PWM control.  
 * Mapping ignores speed values that are too low to make the motor turn.
 * In this case, anything below 27, but 0 still means 0 to stop the motors.
 */
void Motor(char mot, char dir, int speed)
{
  if (speed > 0){ // Update ackData payload with motor speed
    if (mot == 'A'){
      ackData[0] = speed;
    }
    else if (mot = 'B'){
      ackData[1] = speed;
    }
  }
  else if (speed < 0){
    if (mot == 'A'){
      ackData[0] = speed * -1;
    }
    else if (mot == 'B'){
      ackData[1] = speed * -1;
    }
  }
  // remap the speed from range 0-100 to 0-255
  int newspeed;
  if (speed == 0)
    newspeed = 0;   // Don't remap zero, but remap everything else.
  else
    newspeed = map(speed, 1, 100, MIN_SPEED, 255);

  switch (mot) {
    case 'A':   // Controlling Motor A
      if (dir == 'F') {
        digitalWrite(INA, HIGH);
      }
      else if (dir == 'R') {
        digitalWrite(INB, LOW);
      }
      analogWrite(ENA, newspeed);
      break;

    case 'B':   // Controlling Motor B
      if (dir == 'F') {
        digitalWrite(INB, HIGH);
      }
      else if (dir == 'R') {
        digitalWrite(INB, LOW);
      }
      analogWrite(ENB, newspeed);
      break;

    case 'C':  // Controlling Both Motors
      if (dir == 'F') {
        digitalWrite(INA, HIGH);
        digitalWrite(INB, HIGH);
      }
      else if (dir == 'R') {
        digitalWrite(INA, LOW);
         digitalWrite(INB, LOW);
      }
      analogWrite(ENA, newspeed);
      analogWrite(ENB, newspeed);
      break;
  }
  // Send what we are doing with the motors out to the Serial Monitor.
  Serial.print ("[Motor Func] Motor: ");
  if (mot=='C')
      Serial.print ("Both");
    else
      Serial.print (mot);
  Serial.print (" Direction: ");
  Serial.print (dir);
  Serial.print (" Speed: ");
  Serial.print (speed);
  Serial.print (" Mapped Speed: ");
  Serial.println (newspeed);
}
