#include <SPI.h>
#include <printf.h>
#include <RF24.h>
#include <Servo.h>

// RF24 settings and IO pin assignments
byte ChannelFrequency = 24; //!!!!!!!!!!!!!!  Frequency used by transmitter = 2,400mhz + ChannelFrequency.  Must be between 0 and 124 to work.  MUst be between 0 and 83 to stay legal.  Must match on both transceivers.
byte RFpipe=0;            //!!!!!!!!!!!!!!  This is the pipe used to receive data.  Choose a number between 0 and 15.  Must match on both transceivers.

int RF_CE=9;
int RF_CSN = 10;
const byte thisSlaveAddress[] = {'R', 'x', 'A', 'A', 'A', 'A'};

RF24 radio(RF_CE, RF_CSN ); // using pin 7 for the CE pin, and pin 8 for the CSN pin.  9 and 10 for joystick board

uint8_t address[][16] = {"1Node", "2Node", "3Node", "4Node", "5Node", "6Node", "7Node", "8Node", "9Node", "10Node", "11Node", "12Node", "13Node", "14Node", "15Node", "16Node"};
int payload[8];  // array to hold received data.  See transmitter code to view which array elements contain analog and digitial button data. 
int ackData[2] = {109, -4000}; // the two values to send back to the remote, just using random numbers for now
bool newData = false;
// L298P Motor Shield Portion, copied from https://protosupplies.com
//  Motor A
int const BUZZER = 4;
int const ENA = 5;  
int const INA = 3;
//  Motor B
int const ENB = 6;  
int const INB = 2;
int const MIN_SPEED = 27;   // Set to minimum PWM value that will make motors turn
int const ACCEL_DELAY = 50; // delay between steps when ramping motor speed up or down.
// End L298P
/*
// motor control io pin assignments
int M1pwmPin = 5;     //IOpin assignment, enable for motor1 (pwm on this pin). IO pin10 is default, but changed to leave SPI port available
int M1dirPin = 3;     //IOpin assignment, direction for motor1.  IO pin12 is default

int M2pwmPin = 6;     //IOpin assignment, enable for motor2 (pwm on this pin). IO pin11 is default, but changed to leave SPI port available
int M2dirPin = 2;     //IOpin assignment, direction for motor2. IO pin13 is default

int M1speed=255; // variable holding PWM (Pulse Width Modulation) speed of motor, value of 0-255
int M2speed=255; // variable holding PWM (Pulse Width Modulation) speed of motor, value of 0-255
*/
// Hobby Servo io pin assignments
int Svo1pin = A0;
Servo Servo1; // create instance of servo

int retry=0;

void setup() {
  // L298P Motor Shield Portion
  pinMode(ENA, OUTPUT);   // set all the motor control pins to outputs
  pinMode(ENB, OUTPUT);
  pinMode(INA, OUTPUT);
  pinMode(INB, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  // End 298P Portion

  Servo1.attach(Svo1pin);
  Serial.begin(115200);
  while (!Serial) {
    // some boards need to wait to ensure access to serial over USB
  }
  if (!radio.begin()) {
    Serial.println(F("radio hardware is not responding!!"));
    while (1) {} // hold in infinite loop
  }
  radio.setChannel(ChannelFrequency);  // sets the frequency between 2,400mhz and 2,524mhz in 1mhz incriments
  radio.setPALevel(RF24_PA_MAX);  // RF24_PA_MAX is default.
  radio.setPayloadSize(sizeof(payload));  
  radio.openReadingPipe(1, address[RFpipe]);
  radio.enableAckPayload(); // Enables sending data back to transmitter
  radio.startListening(); // put radio in RX mode
  printf_begin();             // needed only once for printing details
  radio.printPrettyDetails(); // (larger) function that prints human readable data
  pinMode(4, OUTPUT);
  radio.writeAckPayload(1, &ackData, sizeof(ackData)); // pre-load data
  payload[0]=0;   // this code puts in default values for the payload
  payload[1]=0;
  for(int x=2; x<8; x++){
    payload[x]=1;
  }

  
  } 

void(* resetFunc) (void) = 0;//declare reset function at address 0


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
        payload[0]=0;
        payload[1]=0;
        for(int x=2; x<8; x++){
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

    if (payload[1]>50){
       /*digitalWrite(M1dirPin,HIGH);
       digitalWrite(M2dirPin,LOW);
       analogWrite(M1pwmPin, M1speed);
       analogWrite(M2pwmPin, M2speed); */
       Motor('A', 'F', payload); // Set Motor A to go forward and speed of payload
       Motor('B', 'R', payload); // Set Motor B to go reverse and speed of payload
       
    }
    else if (payload[1]< -50){
       /*digitalWrite(M1dirPin,LOW);
       digitalWrite(M2dirPin,HIGH);
       analogWrite(M1pwmPin, M1speed);
       analogWrite(M2pwmPin, M2speed); */
       Motor('A', 'F', payload); // Set Motor A to forward and speed of payload
       Motor('B', 'R', payload); // Set Motor B to reverse and speed of payload
    }
    else if (payload[0]> 50){
       /*digitalWrite(M1dirPin,LOW);
       digitalWrite(M2dirPin,LOW);
       analogWrite(M1pwmPin, M1speed);
       analogWrite(M2pwmPin, M2speed); */
       Motor('A', 'R', payload);
       Motor('B', 'R', payload);
    }
    else if (payload[0]< -50){
       /*digitalWrite(M1dirPin,HIGH);
       digitalWrite(M2dirPin,HIGH);
       analogWrite(M1pwmPin, M1speed);
       analogWrite(M2pwmPin, M2speed);*/
       Motor('A', 'F', payload);
       Motor('B', 'F', payload);
    }
    else {
       /*analogWrite(M1pwmPin, 0);  //pwm value of 0 is stopped
       analogWrite(M2pwmPin, 0);  //pwm value of 0 is stopped*/
       Motor('A', 'F', 0); // "0" is to set motor to stop
       Motor('B', 'F', 0); // "0" is to set motor to stop
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
  if (goodSignal == true) { // Check the signal strength and write "1" if it si good, and "0" if its bad to ackData[0]
    ackData[0] = 1;
  }
  else {
    ackData[0] = 0;
  }
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
  Serial.print ("t Direction: ");
  Serial.print (dir);
  Serial.print ("t Speed: ");
  Serial.print (speed);
  Serial.print ("t Mapped Speed: ");
  Serial.println (newspeed);
}
