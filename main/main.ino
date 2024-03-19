void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
}
int ADvalueX;
int ADvalueY;

void loop() {
  // put your main code here, to run repeatedly:
  ADvalueX=analogRead(A0);
  ADvalueY=analogRead(A1);
  Serial.print("X=");
  Serial.print(ADvalueX);
  Serial.print(" Y=");
  Serial.println(ADvalueY);
  delay(10);
}
