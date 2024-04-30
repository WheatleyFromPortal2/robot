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

// RF24 settings and IO pin assignments
byte ChannelFrequency = 1; //!!!!!!!!!!!!!!  Frequency used by transmitter = 2,400mhz + ChannelFrequency.  Must be between 0 and 124 to work.  MUst be between 0 and 83 to stay legal.  Must match on both transceivers.
byte RFpipe=1;            //!!!!!!!!!!!!!!  This is the pipe used to receive data.  Choose a number between 0 and 15.  Must match on both transceivers.

int RF_CE=9;
int RF_CSN = 10;

RF24 radio(RF_CE, RF_CSN ); // using pin 7 for the CE pin, and pin 8 for the CSN pin.  9 and 10 for joystick board

uint8_t address[][16] = {"1Node", "2Node", "3Node", "4Node", "5Node", "6Node", "7Node", "8Node", "9Node", "10Node", "11Node", "12Node", "13Node", "14Node", "15Node", "16Node"};
int payload[8];  // array to hold received data.  See transmitter code to view which array elements contain analog and digitial button data. 


// motor control io pin assignments
int M1pwmPin = 5;     //IOpin assignment, enable for motor1 (pwm on this pin). IO pin10 is default, but changed to leave SPI port available
int M1dirPin = 3;     //IOpin assignment, direction for motor1.  IO pin12 is default

int M2pwmPin = 6;     //IOpin assignment, enable for motor2 (pwm on this pin). IO pin11 is default, but changed to leave SPI port available
int M2dirPin = 2;     //IOpin assignment, direction for motor2. IO pin13 is default

int M1speed=255; // variable holding PWM (Pulse Width Modulation) speed of motor, value of 0-255
int M2speed=255; // variable holding PWM (Pulse Width Modulation) speed of motor, value of 0-255


// Hobby Servo io pin assignments
int Svo1pin = A0;
Servo Servo1; // create instance of servo

int retry=0;


void setup() {
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
  radio.startListening(); // put radio in RX mode
  printf_begin();             // needed only once for printing details
  radio.printPrettyDetails(); // (larger) function that prints human readable data
  pinMode(4, OUTPUT);

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
       digitalWrite(M1dirPin,HIGH);
       digitalWrite(M2dirPin,LOW);
       analogWrite(M1pwmPin, M1speed);
       analogWrite(M2pwmPin, M2speed);
    }
    else if (payload[1]< -50){
       digitalWrite(M1dirPin,LOW);
       digitalWrite(M2dirPin,HIGH);
       analogWrite(M1pwmPin, M1speed);
       analogWrite(M2pwmPin, M2speed);
    }
    else if (payload[0]> 50){
       digitalWrite(M1dirPin,LOW);
       digitalWrite(M2dirPin,LOW);
       analogWrite(M1pwmPin, M1speed);
       analogWrite(M2pwmPin, M2speed);
    }
    else if (payload[0]< -50){
       digitalWrite(M1dirPin,HIGH);
       digitalWrite(M2dirPin,HIGH);
       analogWrite(M1pwmPin, M1speed);
       analogWrite(M2pwmPin, M2speed);
    }
    else {
       analogWrite(M1pwmPin, 0);  //pwm value of 0 is stopped
       analogWrite(M2pwmPin, 0);  //pwm value of 0 is stopped
    }


    // These last lines show how to make hobby servo go to a position when button press is received
    
    if (payload[2]==0) Servo1.write(10);  //send test hobby servo to position 10 when button A is pressed
    else Servo1.write(180);               //send test hobby servo to position 180 When button A is not pressed

    if (payload[3]==0) digitalWrite(4, HIGH);  //turn on buzzer
    else digitalWrite(4, LOW);
    

}
