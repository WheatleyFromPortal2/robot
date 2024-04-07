/* 
This code is for an Arduino Uno transmitter with joystick control board and nRF24 transceiver.

Note the ChannelFrequency and RFpipe variables.  Change this so your transmitter and receiver use the same number so they are paired together.

Note that the serial monitor should be set to 115200bps to monitor the serial.print() output. 

You need to install the library titled "RF24" from the library manager.  "RF24" by "TMRh20,Avamander"
You need to install the library titled "LCD_I2C" from the library manager. "LCD_I2C" by "Blackhack"

v2.2 adds I2C LCD display for 16x2 LCD using I2C controller board.  Requires "LCD_I2C" library by "Blackhack".
I2C LCD control board uses only 4 pins.  Gnd, +5v, Pin A4 (SDA), Pin A5 (SCL).
*/

#include <SPI.h>  // (SPI bus uses hardware IO pins 11, 12, 13)
#include <printf.h>
#include <RF24.h>
// ---Code for U8g2 Display---
#include <U8g2lib.h>
U8G2_SSD1306_128X64_NONAME_1_HW_I2C u8g2(U8G2_R2); // define OLED display
// ---Varaibles needed to be transmitted by the Robot--- (U8g2 display)
int motorA=28;
int motorB=67;
int x1=10; // distance values from distance sensors. x1-x3
int x2=20;
int x3=30; 
int servo1=20;
int servo2 =160;
// ___end of varibles tranmittted by robot___
/* ---Code for old Ardafruit Display---
//#include <Adafruit_GFX.h> // include library for 0.96" OLED display
//#include <Adafruit_SSD1306.h>  // code copied from https://electropeak.com/learn/interfacing-0-96-inch-ssd1306-oled-i2c-display-with-arduino/
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display heigh, in pixels
#define OLED_RESET -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SSD1306_NO_SPLASH // Disable OLED splash-screen
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET); // Declare display
   ___end of code for adafruit display___ */
bool LCDinstalled = false; // if LCD installed, make this true

// io pin assignments for joystick shield
int AnalogX = A0;
int AnalogY = A1;
int ButtonA = 2;
int ButtonB = 3;
int ButtonC = 4;
int ButtonD = 5;
int ButtonE = 6;
int ButtonF = 7;

// RF24 settings and IO pin assignments
byte ChannelFrequency = 24;  // !!! Frequency used by transmitter = 2,400mhz + ChannelFrequency.  Must be between 0 and 83 to be legal. Must match on both transceivers.
byte RFpipe = 0;            // !!! This is the pipe used to receive data.  Choose a number between 0 and 15.  Must match on both transceivers.

int RF_CE = 9;
int RF_CSN = 10;

String buf = String(ChannelFrequency);
// Variables for recieving data from Robot, using ackData

int ackData[8] = {-1, -1, -1, -1, -1, -1, -1, -1}; // -1 is just to test, check README for info on what each is for
bool newData = false;
unsigned long currentMillis;
unsigned long prevMillis;
unsigned long txIntervalMillis = 1000; // send once per second
unsigned long gfxTime;
RF24 radio(RF_CE, RF_CSN);  // using pin 7 for the CE pin, and pin 8 for the CSN pin.  9 and 10 for joystick board

uint8_t address[][16] = { "1Node", "2Node", "3Node", "4Node", "5Node", "6Node", "7Node", "8Node", "9Node", "10Node", "11Node", "12Node", "13Node", "14Node", "15Node", "16Node" };  // 0 to 15

int payload[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
long unsigned int TimeNow;
long unsigned int TimeNext;
long unsigned int successfulTx;
long unsigned int failedTx;
bool lastTxSuccess;
bool lastRxSuccess;
int txPercent;
int displayMode=1;
int vScreen;
bool goodSignal;
void setup() {

  Serial.begin(115200);
  while (!Serial) {
    // some boards need to wait to ensure access to serial over USB
  }
  /*if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x32
    Serial.println(F("SSD1306 allocation failed, reboot Ardiuno to fix, or check I2C connection"));
    for(;;); // hold in infinite loop
  }*/ // Old display code
  Serial.print("ChannelFrequency=");
  Serial.println(ChannelFrequency);
  Serial.print("RFpipe=");
  Serial.println(RFpipe);
  //if (LCDinstalled) SetupLCD();
  u8g2.begin(); // Enables U8g2 display
  if (!radio.begin()) {
    Serial.println(F("radio hardware is not responding.  Please reset."));
    for(;;);  // hold in infinite loop
  }
  radio.setChannel(ChannelFrequency);      // sets the frequency between 2,400mhz and 2,524mhz in 1mhz incriments
  radio.setPALevel(RF24_PA_MAX);           // RF24_PA_MAX is default.
  radio.setPayloadSize(sizeof(payload));   // float datatype occupies 4 bytes
  radio.openWritingPipe(address[RFpipe]);  // set the transmit pipe to use
  radio.stopListening();                   // put radio in TX mode
}  // setup

void loop() {
  currentMillis = millis();
  payload[0] = ((analogRead(AnalogX) - 512) / 5.12);  // read the joystick and button inputs into the payload array.  The math turns the 0-1023 AD value of the analog input to a -100 to +100 number, with 0 (or close to that) being the center position of the joystick.
  payload[1] = ((analogRead(AnalogY) - 512) / 5.12);  // read the joystick and button inputs into the payload array.  The math turns the 0-1023 AD value of the analog input to a -100 to +100 number, with 0 (or close to that) being the center position of the joystick.
  payload[2] = digitalRead(ButtonA);
  payload[3] = digitalRead(ButtonB);
  payload[4] = digitalRead(ButtonC);
  payload[5] = digitalRead(ButtonD);
  payload[6] = digitalRead(ButtonE);
  payload[7] = digitalRead(ButtonF);
  if (currentMillis - prevMillis >= txIntervalMillis) {
    send();
  }
  if (newData == true) { // Print ackData and reset newData
    Serial.println("--Acknowledge data--");
    Serial.println(ackData[0]);
    Serial.println(", ");
    Serial.println(ackData[1]);
    Serial.println(); // Print blank newline
    newData = false;
  }
  // find signal strength
  goodSignal = radio.testRPD();
  if(radio.available()){
  Serial.println(goodSignal ? "Strong signal > -64dBm" : "Weak signal < -64dBm" );

  radio.read(&payload,sizeof(payload));
  }
  if(digitalRead(6)==0){
    displayMode++;
    displayMode = displayMode%2;
  }
  if (LCDinstalled) PrintToLCD();
  delay(50 - gfxTime);  // slow transmissions down by 50ms, accounting for time it takes to render to the display
}

void send() {
  bool report = radio.write(&payload, sizeof(payload));  // transmit the data and receive confirmation report

  for (int x = 0; x <= 7; x++) {  // print out all the data to serial monitor
    Serial.print(payload[x]);
    Serial.print(" ");
  }
  if (report) {
    Serial.println(F("Transmission successful! Sent: "));  // payload was delivered
    successfulTx ++; // Add one to value of successfulTx, since the transmission succeeded
    lastTxSuccess = true;
    if ( radio.isAckPayloadAvailable() ) {
      Serial.println("ackData is available!");
      radio.read(&ackData, sizeof(ackData));
      newData = true;
      lastRxSuccess = true;
    }
    else {
      Serial.println(" Acknowledge but no ackData ");
      lastRxSuccess = false;
    }
  } else {
    Serial.println(F("Transmission failed or timed out"));  // payload was not delivered
    failedTx ++ ; // Add one to value of failedTx, since the transmission failed
    lastTxSuccess = false;
  }
  prevMillis = millis();
}
/*void SetupLCD(){ // !!!OLD CODE for Adafruit!!! Initializes display, YOU MUST CALL "display.display();" AFTER MAKING CHANGES TO UPDATE THE DISPLAY!
  display.display();
  delay(1000); // Pause for 1 second (maybe try lower value later?)
  display.clearDisplay();
  display.setTextSize(1); // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0); // Start at top-left corner
  display.cp437(true); // Use full 256 char 'Code Page 437' font
  // ---Boot-Message---
  buf = String(ChannelFrequency);
  display.println("rfFreq: ");
  display.print(buf); // Prints the frequency
  buf = String(RFpipe);
  display.println("rfPipe: ");
  display.print(buf); // Prints the rfPipe
  buf = String(txIntervalMillis);
  display.println("txIntervalMillis: ");
  display.print(buf);
  display.display();
  delay(1000); // delay 1 second (Maybe lower later?)
}*/

void PrintToLCD() {
  gfxTime = millis();
  txPercent = (successfulTx / (successfulTx + failedTx)) * 100.0; // Calculate the successful Tx percentage, force floating point math

  if (vScreen == 0){ // Prints the !!OLD!! screen, add more else-ifs for more screens!
    /*display.clearDisplay();
    display.setCursor(0,0);
    display.println("Pkt:");
    for (int x = 0; x <= 7; x++) { // Print out payload to OLED
      buf = String(payload[x]);
      display.print(" ");
      display.print(buf);
    }
    buf = String(successfulTx);
    display.println("Tx: ");
    display.print(buf);
    display.print("/");
    buf = String(failedTx);
    display.print(buf);
    display.print(", ");
    buf = String(txPercent);
    display.print(buf);
    display.print("%");
    display.println("ackData:");
    for (int x = 0; x <= 1; x++) { // Print out ackData to OLED
      buf = ackData[x];
      display.print(" ");
      display.print(buf);
    }
    if (goodSignal) {
      display.println("Signal Good! (>-64dBm");
      display.invertDisplay(false); // Don't invert display if the signal is good
    }
    else {
      display.println("Signal Bad! (<-64dBm");
      display.invertDisplay(true); // Inverts entire display if signal is bad
    }*/
  }
    else if (vScreen == 1){
    // this displays the proximity data
     u8g2.drawEllipse(64, 63, 60, 54, U8G2_DRAW_UPPER_RIGHT | U8G2_DRAW_UPPER_LEFT); 
     u8g2.setFont(u8g2_font_profont11_mf);
     u8g2.setCursor(0, 7);
     u8g2.print(x1);
     u8g2.setCursor(58, 7);
     u8g2.print(x2);
     u8g2.setCursor(116, 7);
     u8g2.print(x3);
     u8g2.drawDisc(63, 60, 2); 
     if (x1<40){
       u8g2.drawTriangle(56-(x1*1.3), 63, 56-(x1*1.3), 51, 56-(x1*1.3)+6, 57);
     }
     if (x3<40){
       u8g2.drawTriangle(68+(x3*1.3), 63, 68+(x3*1.3), 51, 68+(x3*1.3)-6, 57);

     }
     if (x2<40){
       u8g2.drawTriangle(63, 60-(x2*1.15), 57, 54-(x2*1.15), 69, 54-(x2*1.15)); 
     }
   }
   else if (vScreen==2){
     // this displays a bar graph (most likely motor speed)//*
     int graphH1= motorA*.48;
     int graphH2= motorB* .48;
     u8g2.drawFrame(0, 12, 12, 52); 
     u8g2.drawBox(2, 62-graphH1, 8, graphH1);

     u8g2.drawFrame(18, 12, 12, 52); 
     u8g2.drawBox(20, 62-graphH2, 8, graphH2);

     u8g2.setFont(u8g2_font_profont11_mf);
     u8g2.setCursor(1, 7);
     u8g2.print("M1");
     u8g2.setCursor(19, 7);
     u8g2.print("M2");
     // draws a dial for servo position
     u8g2.drawCircle(55, 26, 15, U8G2_DRAW_UPPER_RIGHT | U8G2_DRAW_UPPER_LEFT); 
     u8g2.drawDisc(55, 26, 2); 
     u8g2.drawCircle(55, 60, 15, U8G2_DRAW_UPPER_RIGHT | U8G2_DRAW_UPPER_LEFT); 
     u8g2.drawDisc(55, 60, 2); 
     
     int s1x = 17* cos(radians(180-servo1)) +55; 
     int s2x= 17* cos(radians(180-servo2)) + 55; 
     int s1y = 17 * -sin(radians(180-servo1))+ 26;
     int s2y= 17 * -sin(radians(180-servo2)) +60;
     ///*
     int e1x= 5 * cos(radians(360-servo1))+55;
     int e2x=5 * cos(radians(360-servo2))+55;
     int e1y=5 * -sin(radians(360-servo1))+26;
     int e2y=5 * -sin(radians(360-servo2)) +60;
     //*/
     u8g2.drawLine(s1x, s1y, e1x, e1y); 
     u8g2.drawLine(s2x, s2y, e2x, e2y); 

     u8g2.setCursor(50, 7);
     u8g2.print("S1");
     u8g2.setCursor(50, 42);
     u8g2.print("S2");
     // print status of communication
     u8g2.setCursor(80,7);
     if (lastTxSuccess){
       u8g2.print("TX: OK");
     }
     else u8g2.print("TX: fail");
     u8g2.setCursor(80,17);
     if (lastRxSuccess){
       u8g2.print("RX: OK");
     }
     else u8g2.print("RX: fail");
   
  }
  gfxTime = millis() - gfxTime;
  Serial.println("gfxTime: ");
  Serial.print(gfxTime);
}
