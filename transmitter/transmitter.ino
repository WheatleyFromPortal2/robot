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
#include <string.h>
#include <RF24.h>

// #include <LCD_I2C.h>     // include library, more info at https://github.com/blackhack/LCD_I2C
#include <Adafruit_GFX.h> // include library for 0.96" OLED display
#include <Adafruit_SSD1306.h>  // code copied from https://electropeak.com/learn/interfacing-0-96-inch-ssd1306-oled-i2c-display-with-arduino/
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display heigh, in pixels
#define OLED_RESET 4 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SSD1306_NO_SPLASH // Disable OLED splash-screen
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET); // Declare display

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
byte RFpipe = 1;            // !!! This is the pipe used to receive data.  Choose a number between 0 and 15.  Must match on both transceivers.

int RF_CE = 9;
int RF_CSN = 10;

String buf = String(ChannelFrequency);
// Variables for recieving data from Robot, using ackData

int ackData[2] = {-1, -1};
bool newData = false;
unsigned long currentMillis;
unsigned long prevMillis;
unsigned long txIntervalMillis = 1000; // send once per second

RF24 radio(RF_CE, RF_CSN);  // using pin 7 for the CE pin, and pin 8 for the CSN pin.  9 and 10 for joystick board

uint8_t address[][16] = { "1Node", "2Node", "3Node", "4Node", "5Node", "6Node", "7Node", "8Node", "9Node", "10Node", "11Node", "12Node", "13Node", "14Node", "15Node", "16Node" };  // 0 to 15

int payload[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };

long unsigned int TimeNow;
long unsigned int TimeNext;
long unsigned int successfulTx;
long unsigned int failedTx;
int txPercent;
bool goodSignal;
void setup() {

  Serial.begin(115200);
  while (!Serial) {
    // some boards need to wait to ensure access to serial over USB
  }
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x32
    Serial.println(F("SSD1306 allocation failed, reboot Ardiuno to fix, or check I2C connection"));
    for(;;); // hold in infinite loop
  }
  Serial.print("ChannelFrequency=");
  Serial.println(ChannelFrequency);
  Serial.print("RFpipe=");
  Serial.println(RFpipe);
  if (LCDinstalled) SetupLCD();

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
  if (LCDinstalled) PrintToLCD();
  delay(50);  // slow transmissions down by 100ms
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
    if ( radio.isAckPayloadAvailable() ) {
      Serial.println("ackData is available!");
      radio.read(&ackData, sizeof(ackData));
      newData = true;
    }
    else {
      Serial.println(" Acknowledge but no ackData ");
    }
  } else {
    Serial.println(F("Transmission failed or timed out"));  // payload was not delivered
    failedTx ++ ; // Add one to value of failedTx, since the transmission failed
  }
  prevMillis = millis();
}

void SetupLCD(){ // Initializes display, YOU MUST CALL "display.display();" AFTER MAKING CHANGES TO UPDATE THE DISPLAY!
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
  delay(1000); // delay 1second (Maybe lower later?)
}

void PrintToLCD() {
  txPercent = (successfulTx / (successfulTx + failedTx)) * 100.0; // Calculate the successful Tx percentage, force floating point math
  display.clearDisplay();
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
  }
}
