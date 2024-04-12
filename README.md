# This is for the 2024 Eng8 Robot
# [Robot Code](robot.ino)
# [Transmitter Code](transmitter/transmitter.ino)
# Members
- Will Behnke
- William Braskamp
- Hamzah Salman
- Hunter Dorrill
# Hardware
- 2 Arduino Unos (or compatable clones)
- 2 nRF24L01
- [Fundiuno Shield](https://cb-electronics.com/products/funduino-joystick-shield-v1-a-ky-023-shield/)
- [L298P Motor Shield](https://protosupplies.com/product/l298p-motor-driver-shield/)
# Transmitter
- Ardiuno Uno
- [Fundiuno Shield](https://cb-electronics.com/products/funduino-joystick-shield-v1-a-ky-023-shield/)
- nRF24L01
## Pin Assignments
- Joystick X = `A0`
- Joystick Y =`A1`
- Button A = `2`
- Button B = `3`
- Button C = `4`
- Button D = `5`
- Button E = `6`
- Button F = `7`
# Robot
- Ardiuno Uno
- Motor Shield
- 2 DC Motors
- nRF24L01
## Pin Assignments
- BUZZER = `4`
- Servo1 = `7`
- Servo2 = `8`
### Distance Sensors
- x1(Leftmost Dst Sensor) = `A0`
- x2(Middle Dst Sensor) = `A1`
- x3(Rightmost Dst Sensor) = `A2`
- Trigger(for activating dst sensors) = `A3`
### Motor Shield Pins
- ENA = `5`
- INA = `3`
- ENB = `6`
- INB = `2`
## Motor Shield Docs
The shield is built to directly interface pins `10, 11, 12, 13` to the motor control ic for pwm control.  However, those exact pins are the hardware SPI interface for the wireless module **so we must use different motor control pins.**

Since the arduino uno has hardware PWM on pins `3, 5, 6, 9, 10, 11` we can use these pins instead by bending the shield pins to the side so we can connect them manually to the arduino.
- [Ardiuno](https://www.arduino.cc/reference/en/language/functions/analog-io/analogwrite/)
- [L298P Motor Driver Instructables](https://www.instructables.com/Tutorial-for-L298-2Amp-Motor-Driver-Shield-for-Ard/)
- [Electropeak Tutorial](https://electropeak.com/learn/interfacing-l298p-h-bridge-motor-driver-shield-with-arduino/)
- [Hands On Tech specs](http://www.handsontec.com/dataspecs/arduino-shield/L298P%20Motor%20Shield.pdf)
- [HandsOnTech](HandsOnTec.pdf)
## Wiring
1. nRF24 `pin1` -> Arduino `Gnd`
2. nRF24 `pin2` -> Arduino `+3.3v`.  Note, if you hook it up to +5v it will probably break.
3. nRF24 `pin3` -> Arduino `IOpin9`
4. nRF24 `pin4` -> Arduino `IOpin10`
5. nRF24 `pin5` -> Arduino `IOpin13`
6. nRF24 `pin6` -> Arduino `IOpin11`
7. nRF24 `pin7` -> Arduino `IOpin12`
8. nRF24 `pin8` **is not required** since we are not using interrupts (IRQs), leave this pin disconnected
# Payload Characteristics; Transmitter -> Robot
## Uses an array with **8** elements, numbered 0-7
0. Value of `Joystick X` from -100 to 100
1. Value of `Joystick Y` from -100 to 100
2. Value of `Button A`
3. Value of `Button B`
4. Value of `Button C`
5. Value of `Button D`
6. Value of `Button E`
7. Value of `Button F`
# ackData Characteristics; Robot -> Transmitter
## Uses an array of **8** elements, numbered 0-7
0. `motorA` Motor A power percent (0-100)
1. `motorB` Motor B power percent (0-100)
2. `x1` Distance value for leftmost distance sensor (centimeters)
3. `x2` Distance value for middle distance sensor (centimeters)
4. `x3` Distance value for rightmost distance sensor (centimeters)
5. `servo1` Rotation of leftmost servo (degrees)
6. `servo2` Rotation of rightmost servo (degrees)
7. `ackConnect` Value of connection quality from robot (0-1)
