#include <Servo.h>
Servo Xservo;

void setup() {
  Serial.begin(9600);
  Xservo.attach(9);
}

int ADvalueX;
int Xpos;
int ADvalueY;

void loop() {
  ADvalueX=analogRead(A0);
  Xpos = ADvalueX * 180.0 / 1023.0; // extra zeros to force floating point math
  ADvalueY=analogRead(A1);
  Serial.print("X=");
  Serial.print(ADvalueX);
  Serial.print(" Xpos=");
  Serial.print(Xpos);
  Serial.print(" Y=");
  Serial.println(ADvalueY);
  delay(10);
  
  Xservo.write(Xpos);

  //Xservo.write(0);
  //delay(1000);
  //Xservo.write(180);
  //delay(1000);
}
