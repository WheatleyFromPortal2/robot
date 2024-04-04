#include <Servo.h>
Servo Xservo;
Servo Yservo;

// Motor code copied from protosupplies.com

// L298P Motor Shield Portion
//  Motor A
int const BUZZER = 4;
int const ENA = 10;  
int const INA = 12;
//  Motor B
int const ENB = 11;  
int const INB = 13;
int const MIN_SPEED = 27;   // Set to minimum PWM value that will make motors turn
int const ACCEL_DELAY = 50; // delay between steps when ramping motor speed up or down.
// End L298P

void setup() {
  Serial.begin(9600);
  Xservo.attach(9);
  Yservo.attach(10);

  // L298P Motor Shield Portion
  pinMode(ENA, OUTPUT);   // set all the motor control pins to outputs
  pinMode(ENB, OUTPUT);
  pinMode(INA, OUTPUT);
  pinMode(INB, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  // End 298P Portion
}

int ADvalueX;
int Xpos;
int ADvalueY;
int Ypos;

void loop() {
  ADvalueX=analogRead(A0);
  ADvalueY=analogRead(A1);
  if (ADvalueX > 512) {
    ADvalueX = ADvalueX - 512;
    Xpos = ADvalueX * 100.0 / 512.0;
    Motor('A', 'F', Xpos);
  }
  else if (ADvalueX < 512) {
    Xpos = ADvalueX * 100.0 / 512.0;
    Motor('A', 'R', Xpos);
  }
  else {
    Motor('A', 'F', 0);
  }
  if (ADvalueY > 512) {
    ADvalueY = ADvalueY - 512;
    Ypos = ADvalueY * 100.0 / 512.0;
    Motor('B', 'F', Ypos);
  }
  else if (ADvalueY < 512) {
    Ypos = ADvalueY * 100.0 / 512.0;
    Motor('B', 'R', Ypos);
  }
  else {
    Motor('B', 'F', 0);
  }
  //Xpos = ADvalueX * 100.0 / 1023.0; // extra zeros to force floating point math
  //Ypos = ADvalueY * 100.0 / 1023.0;
  Serial.print("X=");
  Serial.print(ADvalueX);
  Serial.print(" Xpos=");
  Serial.print(Xpos);
  Serial.print(" Y=");
  Serial.println(ADvalueY);
  delay(10);  
  //Xservo.write(Xpos);
  //Yservo.write(Ypos);
  Motor('A', 'F', Xpos);
  Motor('B', 'F', Xpos);

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
  
  Serial.print ("[Motor Func]Motor: ");
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
