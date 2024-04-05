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

#include <LCD_I2C.h>     // include library, more info at https://github.com/blackhack/LCD_I2C
LCD_I2C MyLCD(0x27);     // create object called MyLCD and set default address of most PCF8574 modules
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

RF24 radio(RF_CE, RF_CSN);  // using pin 7 for the CE pin, and pin 8 for the CSN pin.  9 and 10 for joystick board

uint8_t address[][16] = { "1Node", "2Node", "3Node", "4Node", "5Node", "6Node", "7Node", "8Node", "9Node", "10Node", "11Node", "12Node", "13Node", "14Node", "15Node", "16Node" };  // 0 to 15

int payload[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };

long unsigned int TimeNow;
long unsigned int TimeNext;

void setup() {

  Serial.begin(115200);
  while (!Serial) {
    // some boards need to wait to ensure access to serial over USB
  }
  Serial.print("ChannelFrequency=");
  Serial.println(ChannelFrequency);
  Serial.print("RFpipe=");
  Serial.println(RFpipe);
  if (LCDinstalled) SetupLCD();

  if (!radio.begin()) {
    Serial.println(F("radio hardware is not responding.  Please reset."));
    while (1) {}  // hold in infinite loop
  }
  radio.setChannel(ChannelFrequency);      // sets the frequency between 2,400mhz and 2,524mhz in 1mhz incriments
  radio.setPALevel(RF24_PA_MAX);           // RF24_PA_MAX is default.
  radio.setPayloadSize(sizeof(payload));   // float datatype occupies 4 bytes
  radio.openWritingPipe(address[RFpipe]);  // set the transmit pipe to use
  radio.stopListening();                   // put radio in TX mode

}  // setup

void loop() {
  payload[0] = ((analogRead(AnalogX) - 512) / 5.12);  // read the joystick and button inputs into the payload array.  The math turns the 0-1023 AD value of the analog input to a -100 to +100 number, with 0 (or close to that) being the center position of the joystick.
  payload[1] = ((analogRead(AnalogY) - 512) / 5.12);  // read the joystick and button inputs into the payload array.  The math turns the 0-1023 AD value of the analog input to a -100 to +100 number, with 0 (or close to that) being the center position of the joystick.
  payload[2] = digitalRead(ButtonA);
  payload[3] = digitalRead(ButtonB);
  payload[4] = digitalRead(ButtonC);
  payload[5] = digitalRead(ButtonD);
  payload[6] = digitalRead(ButtonE);
  payload[7] = digitalRead(ButtonF);
  bool report = radio.write(&payload, sizeof(payload));  // transmit the data and receive confirmation report

  for (int x = 0; x <= 7; x++) {  // print out all the data to serial monitor
    Serial.print(payload[x]);
    Serial.print(" ");
  }
  if (report) {
    Serial.println(F("Transmission successful! Sent: "));  // payload was delivered
  } else {
    Serial.println(F("Transmission failed or timed out"));  // payload was not delivered
  }
  // find signal strength
  bool goodSignal = radio.testRPD();
  if(radio.available()){
  Serial.println(goodSignal ? "Strong signal > -64dBm" : "Weak signal < -64dBm" );

  radio.read(&payload,sizeof(payload));
  }
  if (LCDinstalled) PrintToLCD();
  delay(50);  // slow transmissions down by 100ms
}

void SetupLCD(){
    MyLCD.begin();          // initialize the display
    MyLCD.backlight();      // start the backlight
    MyLCD.clear();          // clear the screen
    MyLCD.setCursor(0, 0);  // set cursor position .setCursor(column 0-15, row 0-1)
    MyLCD.print("Chan:");   // print to the screen at the cursor position
    MyLCD.print(ChannelFrequency);
    MyLCD.print(" Pipe:");  // print to the screen at the cursor position
    MyLCD.print(RFpipe);
}

void PrintToLCD() {
  MyLCD.setCursor(0, 1);
  MyLCD.print(payload[0]);
  MyLCD.print("    ");
  MyLCD.setCursor(5, 1);
  MyLCD.print(payload[1]);
  MyLCD.print("    ");
  MyLCD.setCursor(10, 1);
  for (int x = 2; x <= 7; x++) MyLCD.print(payload[x]);
}
