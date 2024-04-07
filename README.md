# This is for the 2024 Eng8 Robot
# [Robot Code](robot.ino)
# [Transmitter Code](transmitter/transmitter.ino)
# Members
- Will Benkhe
- William Braskamp
- Hamzah Salman
- Hunter Dorrill
# Hardware
- 2 Arduino Unos (or compatable clones)
- [Fundiuno Shield](https://cb-electronics.com/products/funduino-joystick-shield-v1-a-ky-023-shield/)
- 9g Micro Servo (IO Pin 9)
# Transmitter
- Uses Fundiuno Shield
- Uses same RF24 Module
## Pin Assignments
- Joystick X = `A0`
- Joystick Y =`A1`
- Button A = `2`
- Button B = `3`
- Button C = `4`
- Button D = `5`
- Button E = `6`
- Button F = `7`
## Payload Characteristics
### Uses an array with **8** elements, numbered 0-7
0. Value of Joystick X from -100 to 100
1. Value of Joystick Y from -100 to 100
2. Value of Button A
3. Value of Button B
4. Value of Button C
5. Value of Button D
6. Value of Button E
7. Value of Button F
## ackData Characteristics, used for sending data back to transmitter
### Uses an array of **8** elements, numbered 0-7
0. `motorA` Motor A power percen t (0-100)
1. `motorB` Motor B power percent (0-100)
2. `x1` Distance value for leftmost distance sensor (centimeters)
3. `x2` Distance value for middle distance sensor (centimeters)
4. `x3` Distance value for rightmost distance sensor (centimeters)
5. `servo1` Rotation of leftmost servo (degrees)
6. `servo2` Rotation of rightmost servo (degrees)
7. `ackConnect` Value of connection quality from robot (0-1)
## Fundiuno Schematic
![schematic from cb-electronics.com](schematic.png)
# Motor Shield Docs
The shield is built to directly interface pins 10, 11, 12, 13 to the motor control ic for pwm control.  However, those exact pins are the hardware SPI interface for the wireless module so we must use different motor control pins.


Since the arduino uno has hardware PWM on pins 3, 5, 6, 9, 10, 11, we can use these pins instead by bending the shield pins to the side so we can connect them manually to the arduino.
- [Ardiuno](https://www.arduino.cc/reference/en/language/functions/analog-io/analogwrite/)
- [L298P Motor Driver Instructables](https://www.instructables.com/Tutorial-for-L298-2Amp-Motor-Driver-Shield-for-Ard/)
- [Electropeak Tutorial](https://electropeak.com/learn/interfacing-l298p-h-bridge-motor-driver-shield-with-arduino/)
- [Hands On Tech specs](http://www.handsontec.com/dataspecs/arduino-shield/L298P%20Motor%20Shield.pdf)
- [HandsOnTech](HandsOnTec.pdf)
# Motor Shield Example
## Copied from [Proto Supplies](https://protosupplies.com/product/l298p-motor-driver-shield/)

```
/*
*  L298P Motor Shield
*  Code for exercising the L298P Motor Control portion of the shield
*  The low level motor control logic is kept in the function 'Motor'
*/
// The following pin designations are fixed by the shield
int const BUZZER = 4;
//  Motor A
int const ENA = 10;  
int const INA = 12;
//  Motor B
int const ENB = 11;  
int const INB = 13;

int const MIN_SPEED = 27;   // Set to minimum PWM value that will make motors turn
int const ACCEL_DELAY = 50; // delay between steps when ramping motor speed up or down.
//===============================================================================
//  Initialization
//===============================================================================
void setup()
{
  pinMode(ENA, OUTPUT);   // set all the motor control pins to outputs
  pinMode(ENB, OUTPUT);
  pinMode(INA, OUTPUT);
  pinMode(INB, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  Serial.begin(9600);     // Set comm speed for serial monitor messages
}
//===============================================================================
//  Main
//===============================================================================
void loop()
{
  // Run both motors Forward at 75% power
  Motor('C', 'F', 75);   
  delay(3000);
  
  // Run both motors in Reverse at 75% power but sound beeper first
  Motor('C', 'F', 0);  // Stop motors
  delay(1000);
  digitalWrite(BUZZER,HIGH);delay(500);digitalWrite(BUZZER,LOW); 
  delay(500);
  digitalWrite(BUZZER,HIGH);delay(500);digitalWrite(BUZZER,LOW); 
  delay(1000);
  Motor('C', 'R', 75);  // Run motors forward at 75%
 delay(3000); 
  
  // now run motors in opposite directions at same time at 50% speed
  Motor('A', 'F', 50);
  Motor ('B', 'R', 50);
  delay(3000);
  
  // now turn off both motors
  Motor('C', 'F', 0);  
  delay(3000);

  // Run the motors across the range of possible speeds in both directions
  // Maximum speed is determined by the motor itself and the operating voltage

  // Accelerate from zero to maximum speed
  for (int i = 0; i <= 100; i++)
  {
    Motor('C', 'F', i);
    delay(ACCEL_DELAY);
  }
  delay (2000);
  
  // Decelerate from maximum speed to zero
  for (int i = 100; i >= 0; --i)
  {
    Motor('C', 'F', i);
    delay(ACCEL_DELAY);
  }
  delay (2000);
  
  // Set direction to reverse and accelerate from zero to maximum speed
  for (int i = 0; i <= 100; i++)
  {
    Motor('C', 'R', i);
    delay(ACCEL_DELAY);
  }
  delay (2000);
  
  // Decelerate from maximum speed to zero
  for (int i = 100; i >= 0; --i)
  {
    Motor('C', 'R', i);
    delay(ACCEL_DELAY);
  }
  // Turn off motors
  Motor('C', 'F', 0);
  delay (3000);
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
  
  Serial.print ("Motor: ");
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
```
