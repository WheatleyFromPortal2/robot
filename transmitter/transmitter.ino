/*This code is for an Arduino Uno transmitter with joystick control board and nRF24 transceiver.

Note the ChannelFrequency and RFpipe variables.  Change this so your transmitter and receiver use the same number so they are paired together.
Note the gfxInterval variable, change this to adjust input delay !!!Numbers too low will cause constant errors!!!
Note that the serial monitor should be set to 115200baud to monitor the serial.print() output. 

You need to install the libraries titled:
- "RF24" from the library manager.  "RF24" by "TMRh20,Avamander"
- "U8g2" from the library manager.  "U8g2" by "oliver"<olikraus@gmail.com> */

#include <SPI.h>  // (SPI bus uses hardware IO pins 11, 12, 13)
#include <printf.h>
#include <RF24.h>
#include <U8g2lib.h>
#include <Wire.h>
U8G2_SSD1306_128X64_NONAME_1_HW_I2C u8g2(U8G2_R2);

// ---THESE VARIABLES MUST MATCH ON TRANSMITTER AND ROBOT---
byte ChannelFrequency = 24;  // !!! Frequency used by transmitter = 2,400mhz + ChannelFrequency.  Must be between 0 and 83 to be legal. Must match on both transceivers.
byte RFpipe = 0;             // !!! This is the pipe used to receive data.  Choose a number between 0 and 15.  Must match on both transceivers.
int const gfxInterval = 25; // interval to wait for graphics update, VERY SENSITIVE. Affects input delay greatly
int const llTime = 5; // time to wait during ll(Low Latency) mode
// ---End matching vars---

int ackData[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };  // initialize array with default values
// IO pin assignments for joystick shield
int const AnalogX = A0;
int const AnalogY = A1;
int const joyButton = 8; // button for joystick
int const ButtonA = 2;
int const ButtonB = 3;
int const ButtonC = 4;
int const ButtonD = 5;
int const ButtonE = 6;
int const ButtonF = 7;
float const x1Scale = 0.25; // scaling vars for distance sensor rendering
float const x2Scale = 0.2; // ^
float const x3Scale = 0.25;// |
// RF24 settings and IO pin assignments

int const RF_CE = 9;
int const RF_CSN = 10;

// Variables for recieving data from Robot, using ackData
bool doingLL = false; // controls Low Latency Mode
bool printedLL = false; // use this to find out if we have printed to screen for doingLL

bool newData = false;
unsigned long currentMillis;
unsigned long prevMillis;
unsigned long txIntervalMillis = 20;  // send once per second
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
float txPercent;
int vScreen = 1; // don't recommend to change, 1 is the best vScreen
void setup() {
  Serial.begin(115200);
  while (!Serial) {
    // some boards need to wait to ensure access to serial over USB
  }
  Serial.print(F("ChannelFrequency="));  // "F" is to force reading from flash, to reduce dynamic memory use
  Serial.println(ChannelFrequency);
  Serial.print(F("RFpipe="));
  Serial.println(RFpipe);
  u8g2.begin();  // Enables U8g2 display
  if (!radio.begin()) {
    Serial.println(F("radio hardware is not responding.  Please reset."));
    for (;;)
      ;  // hold in infinite loop
  }
  radio.setDataRate(RF24_250KBPS);
  radio.enableAckPayload();
  radio.setChannel(ChannelFrequency);      // sets the frequency between 2,400mhz and 2,524mhz in 1mhz incriments
  radio.setPALevel(RF24_PA_MAX);           // RF24_PA_MAX is default.
  radio.setPayloadSize(sizeof(payload));   // float datatype occupies 4 bytes
  radio.openWritingPipe(address[RFpipe]);  // set the transmit pipe to use
  //radio.stopListening();                  // put radio in TX mode, maybe this will fix ackData?
}  // setup

void loop() {
  currentMillis = millis();
  if (digitalRead(joyButton) == 0) { // if the joystick button is pressed
    payload[0] = map(analogRead(AnalogX), 0, 1023, -50, 50); // use map() to map 0-1023 ADC Value to -50 to 50 for fine joystick control
    payload[1] = map(analogRead(AnalogY), 0, 1023, -50, 50); // use map() to map 0-1023 ADC Value to -50 to 50 for fine joystick control
  }
  else { // normal operation
    payload[0] = map(analogRead(AnalogX), 0, 1023, -100, 100); // uses map() to map 0-1023 ADC value to -100 to 100 for joystick control
    payload[1] = map(analogRead(AnalogY), 0, 1023, -100, 100); // uses map() to map 0-1023 ADC value to -100 to 100 for joystick control
  }
  //payload[0] = ((analogRead(AnalogX) - 512) / 5.12);  // read the joystick and button inputs into the payload array.  The math turns the 0-1023 AD value of the analog input to a -100 to +100 number, with 0 (or close to that) being the center position of the joystick.
  //payload[1] = ((analogRead(AnalogY) - 512) / 5.12);  // read the joystick and button inputs into the payload array.  The math turns the 0-1023 AD value of the analog input to a -100 to +100 number, with 0 (or close to that) being the center position of the joystick.
  payload[2] = digitalRead(ButtonA);
  payload[3] = digitalRead(ButtonB);
  payload[4] = digitalRead(ButtonC);
  payload[5] = digitalRead(ButtonD);
  payload[6] = digitalRead(joyButton);
  //payload[7] = digitalRead(ButtonF);  // removed, as ButtonF is used for controlling Low Latency Mode
  if (digitalRead(ButtonE) == 0) {
    if (vScreen == -1) {vScreen = 1;} // if there is a graphics reset, go back to vScreen 1; instead of 0, because vScreen0 has too high of a gfxTime(maybe print to multiple lines)
    vScreen += 1;
    if (vScreen > 1) { vScreen = 0; }
  }
  if (digitalRead(ButtonF) == 0) {
    doingLL = !doingLL; // flip doingLL if ButtonF is pressed
    if (doingLL) {payload[7] = 1; printedLL = false;} // make payload[7] = 1, if doingLL is true. make printedLL false to update on screen
    else payload[7] = 0; vScreen = 1; // ^ or else make doingLL = 0 and set vScreen back to normal
  }
  if (currentMillis - prevMillis >= txIntervalMillis) {
    send();
  }
  if (newData == true) {  // Print ackData and reset newData
    for (int i = 0; i <= 7; i++) {
      Serial.print(ackData[i]);
      if (i != 7) Serial.print(",");
      newData = false;
    }
    if (doingLL && printedLL) delay(llTime); // wait by llTIme if doingLL is true
    else {
      if (doingLL) vScreen = -2; // if doingLL, vScreen needs to be -2
      PrintToLCD(); // if not in Low Latency print to OLED
      Serial.println(F("GFXtime: "));
      Serial.println(gfxTime);
      if (gfxTime > gfxInterval) {
        Serial.println(F("[ERROR] GFXtime too long!"));
        vScreen = -1; // set error vScreen
      } else {
        delay(gfxInterval - gfxTime);  // slow transmissions down by gfxInterval ms, accounting for time it takes to render to the display
      }
    }
  }
}
  void send() {
    bool report = radio.write(&payload, sizeof(payload));  // transmit the data and receive confirmation report

    for (int x = 0; x <= 7; x++) {  // print out all the data to serial monitor
      Serial.print(payload[x]);
      Serial.print(F(" "));
    }
    if (report) {
      Serial.println(F("Transmission successful! Sent: "));  // payload was delivered
      successfulTx++;                                        // Add one to value of successfulTx, since the transmission succeeded
      lastTxSuccess = true;
      if (radio.isAckPayloadAvailable()) {
        Serial.println(F("ackData is available!"));
        radio.read(&ackData, sizeof(ackData));
        newData = true;
        lastRxSuccess = true;
      } else {
        Serial.println(F(" Acknowledge but no ackData "));
        lastRxSuccess = false;
      }
    } else {
      Serial.println(F("Transmission failed or timed out"));  // payload was not delivered
      failedTx++;                                             // Add one to value of failedTx, since the transmission failed
      lastTxSuccess = false;
      lastRxSuccess = false;
      PrintToLCD();
    }
    prevMillis = millis();
  }

  void PrintToLCD() {
    u8g2.firstPage();
    do {
      if (vScreen != -1) gfxTime = millis(); // if it is not the "gfxTime too long!" error, find the gfxTime
      txPercent = (float(successfulTx) / float(((successfulTx + failedTx)))) * 100.0;  // Calculate the successful Tx percentage, force floating point math
      if (vScreen == -2) { // print "Low Latency Mode engaged" message
        u8g2.setFont(u8g2_font_profont11_mf);
        u8g2.setCursor(0,8);
        u8g2.print(F("Tx Low Latency On!"));
        u8g2.setCursor(0,18); // move to next line
        //if (ackData[7] == 1) u8g2.print(F("Robot Low Latency On! "));
        //else u8g2.print(F("ERR: Robot not doingLL"));
        //u8g2.setCursor(0,28); // move to next line
        u8g2.print(F("llTime: "));
        u8g2.print(llTime);
        //vScreen = 1; // bring vScreen back to 1 once it returns to normal
        printedLL = true; // ll has now been printed
      }
      if (vScreen == -1) { // Print "gfxTime too long" error message
        u8g2.setFont(u8g2_font_profont11_mf);
        u8g2.setCursor(0,8);
        u8g2.print(F("gfxTime was too long!"));
        u8g2.setCursor(0,18);
        u8g2.print(F("gfxTime: "));
        u8g2.print(gfxTime);
        u8g2.setCursor(0,28);
        u8g2.print(F("Press ButtonE to reset"));
      }
      if (vScreen == 0) {  // Raw Data vScreen
        u8g2.setFont(u8g2_font_profont11_mf);
        //u8g2.clearDisplay();
        u8g2.setCursor(0, 8);
        u8g2.print(F("payload["));  // Print payload
        for (int i = 0; i <= 7; i++) {
          u8g2.print(payload[i]);
          if (i != 7) u8g2.print(F(",")); // print "," if it is not the last index
          if (i == 3) u8g2.setCursor(0, 18); // once halfway through the payload, move the cursor down a line
        }
        u8g2.print(F("]"));
        u8g2.setCursor(0, 28); // move to third line for ackData
        u8g2.print(F("ackData["));  // Print ackData
        for (int i = 0; i <= 7; i++) {
          u8g2.print(ackData[i]);
          if (i != 7) u8g2.print(F(",")); // print "," if it is not the last index
          if (i == 3) u8g2.setCursor(0, 38); // once halfway through ackData, move the cursor down a line
        }
        u8g2.print(F("]"));
        u8g2.setCursor(0, 48); // move down a line
        u8g2.print(F("Tx S/F:"));  // Print Tx Success/Fail and percent
        u8g2.print(successfulTx);
        u8g2.print(F("/"));
        u8g2.print(failedTx);
        u8g2.print(F(" "));
        u8g2.print(txPercent);
        u8g2.print(F("%"));
        //u8g2.setCursor(0, 38);
      } else if (vScreen == 1) {
        // this displays a bar graph (most likely motor speed)//*
        //u8g2.clearDisplay();
        int graphH1 = abs(ackData[0]) * .48;
        int graphH2 = abs(ackData[1]) * .48;
        u8g2.drawFrame(0, 12, 12, 52);
        u8g2.drawBox(2, 62 - graphH1, 8, graphH1);

        u8g2.drawFrame(18, 12, 12, 52);
        u8g2.drawBox(20, 62 - graphH2, 8, graphH2);

        u8g2.setFont(u8g2_font_profont11_mf);
        u8g2.setCursor(1, 7);
        u8g2.print(F("M1"));
        u8g2.setCursor(0, 40);
        if (ackData[0] < 0) u8g2.print(F("/\\"));
        else if (ackData[0] > 0) u8g2.print(F("\\/"));
        u8g2.setCursor(19, 7);
        u8g2.print(F("M2"));
        u8g2.setCursor(18, 40);
        if (ackData[1] > 0) u8g2.print(F("\\/"));
        else if (ackData[1] < 0) u8g2.print(F("/\\"));
        // draws a dial for servo position
        u8g2.drawCircle(55, 26, 15, U8G2_DRAW_UPPER_RIGHT | U8G2_DRAW_UPPER_LEFT);
        u8g2.drawDisc(55, 26, 2);
        u8g2.drawCircle(55, 60, 15, U8G2_DRAW_UPPER_RIGHT | U8G2_DRAW_UPPER_LEFT);
        u8g2.drawDisc(55, 60, 2);

        int s1x = 17 * cos(radians(180 - ackData[5])) + 55;
        int s2x = 17 * cos(radians(180 - ackData[6])) + 55;
        int s1y = 17 * -sin(radians(180 - ackData[5])) + 26;
        int s2y = 17 * -sin(radians(180 - ackData[6])) + 60;
        
        int e1x = 5 * cos(radians(360 - ackData[5])) + 55;
        int e2x = 5 * cos(radians(360 - ackData[6])) + 55;
        int e1y = 5 * -sin(radians(360 - ackData[5])) + 26;
        int e2y = 5 * -sin(radians(360 - ackData[6])) + 60;
        
        u8g2.drawLine(s1x, s1y, e1x, e1y);
        u8g2.drawLine(s2x, s2y, e2x, e2y);

        u8g2.setCursor(50, 7);
        u8g2.print(F("S1"));
        u8g2.setCursor(50, 42);
        u8g2.print(F("S2"));
        // print status of communication
        u8g2.setCursor(80, 7);
        if (lastTxSuccess) {
          u8g2.print(F("Tx: OK"));
        } 
        else u8g2.print(F("Tx: FAIL"));
        u8g2.setCursor(80, 17);
        if (lastRxSuccess) {
          u8g2.print(F("Rx: OK"));
        } else u8g2.print(F("Rx: FAIL"));
        u8g2.setCursor(80, 27);
        u8g2.print(F("Tx S/F:"));
        u8g2.setCursor(80, 37);
        u8g2.print(successfulTx);
        u8g2.print(F("/"));
        u8g2.print(failedTx);
        u8g2.setCursor(80, 47);
        u8g2.print(txPercent);
        u8g2.print(F("%"));
      }
      /* comment out proximity vScreen, it is no lonnger needed
      else if (vScreen == 2) {
        // this displays the proximity data
        u8g2.drawEllipse(64, 63, 60, 54, U8G2_DRAW_UPPER_RIGHT | U8G2_DRAW_UPPER_LEFT);
        u8g2.setFont(u8g2_font_profont11_mf);
        u8g2.setCursor(0, 7);
        u8g2.print(ackData[2]);
        u8g2.setCursor(58, 7);
        u8g2.print(ackData[3]);
        u8g2.setCursor(116, 7);
        u8g2.print(ackData[4]);
        u8g2.drawDisc(63, 60, 2);
        if (ackData[2] < 200) { // display triangle for x1
          u8g2.drawTriangle(56 - (ackData[2] * x1Scale), 63, 56 - (ackData[2] * x1Scale), 51, 56 - (ackData[2] * x1Scale) + 6, 57);
        }
        if (ackData[3] < 200) { // display triangel for x2
          u8g2.drawTriangle(63, 60 - (ackData[3] * x2Scale), 57, 54 - (ackData[3] * x2Scale), 69, 54 - (ackData[3] * x2Scale));
        }
        if (ackData[4] < 200) { // display triangle for x3
          u8g2.drawTriangle(68 + (ackData[4] * x3Scale), 63, 68 + (ackData[4] * x3Scale), 51, 68 + (ackData[4] * x3Scale) - 6, 57);
        }
      }*/
      if (vScreen != -1) gfxTime = millis() - gfxTime; // if it is not the "gfxTime too long!" error, find the gfxTime
      //Serial.println("gfxTime: ");
      //Serial.print(gfxTime);
    } while (u8g2.nextPage());
  }
