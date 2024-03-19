#include <Servo.h>
Servo Xservo;

void setup() {
  Serial.begin(9600);
  Xservo.attach(9);
}

int ADvalueX;
int ADvalueY;

void loop() {
  ADvalueX=analogRead(A0);
  ADvalueY=analogRead(A1);
  Serial.print("X=");
  Serial.print(ADvalueX);
  Serial.print(" Y=");
  Serial.println(ADvalueY);
  delay(10);
  
  Xservo.write(ADvalueX * .176)

  //Xservo.write(0);
  //delay(1000);
  //Xservo.write(180);
  //delay(1000);
}
