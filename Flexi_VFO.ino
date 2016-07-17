

//Code for a flexible VFO for diode matrix tuned radios..

//Includes
#include <LiquidCrystal.h>
#include <Wire.h>
#include <Keypad_MCP.h>
#include <Keypad.h>
#define encoder0PinA  9
#define encoder0PinB  10

volatile unsigned int encoder0Pos = 0;

// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

//set frequancy bytes up - tuning takes 2 bytes 
//currently set at 411.6 mhz for a receve frerquency of 433.0mhz
//i've got it a bit backwards but a zero represents a disconnection of the pin and a 1 is a shoted pin on the diode matrix
//this works for driving the fets that'll be doing the matrix switching but I have to "not" the bits to get the frequency readout right.
//it also means that i constantly get things backwards !!

byte tunehigh = B01111111;
 byte tunelow = B01011111;
int tune = tunehigh << 8 | tunelow;
//init other vars
const int deltime = 200;
long lofreq;
long rxfreq;
int button1;
int button2;
int tunestep = 1;
int ptt;
char key;
const byte  mcp_address=0x24;      // I2C Address of MCP23017 Chip
const byte  GPIOA=0x12;            // Register Address of Port A
const byte  GPIOB=0x13;            // Register Address of Port B

//function to calculate frequency from tuning bytes

long frequency(byte high, byte low){
 long result = 0;
 int N9 = (high >> 7) & 1; //N9
 int N8 = (high >> 6) & 1; //N8
 int N7 = (high >> 5) & 1; //N7
 int N6 = (high >> 4) & 1; //N6
 int N5 = (high >> 3) & 1; //N5
 int N4 = (high >> 2) & 1; //N4
 int N3 = (high >> 1) & 1; //N3
 int N2 = (high >> 0) & 1; //N2
 int N1 = (low >> 7) & 1; //N1
 int N0 = (low >> 6) & 1; //N0
 int A5 = (low >> 5) & 1; //A5
 int A4 = (low >> 4) & 1; //A4
 int A3 = (low >> 3) & 1; //A3
 int A2 = (low >> 2) & 1; //A2
 int A1 = (low >> 1) & 1; //A1
 int A0 = (low >> 0) & 1; //A0

 // the following statements calculate the frequency from the active bits set
 // these are for a tait 555 uhf trx. change the nubers for your specific radio if different
 // had to do it in single hz as only a Long Int would hold the value nicely !!
 
 if (N9 == 0){result = result + 409600000;}
 if (N8 == 0){result = result + 204800000;}
 if (N7 == 0){result = result + 102400000;}
 if (N6 == 0){result = result + 51200000;}
 if (N5 == 0){result = result + 25600000;}
 if (N4 == 0){result = result + 12800000;}
 if (N3 == 0){result = result + 6400000;}
 if (N2 == 0){result = result + 3200000;}
 if (N1 == 0){result = result + 1600000;}
 if (N0 == 0){result = result + 800000;}
 if (A5 == 0){result = result + 400000;}
 if (A4 == 0){result = result + 200000;}
 if (A3 == 0){result = result + 100000;}
 if (A2 == 0){result = result + 50000;}
 if (A1 == 0){result = result + 25000;}
 if (A0 == 0){result = result + 12500;}
 
return result;
 
}

//function to refresh display for RX

void showrx(){
  
lofreq = frequency(tunehigh, tunelow);

lcd.clear();
lcd.print("LO:");
lcd.print(lofreq);
lcd.print("Hz");
lcd.setCursor(0,1);

rxfreq = lofreq + 21400000;

lcd.print("RX:");
lcd.print(rxfreq);
lcd.print("Hz");

if (rxfreq < 433000000){lcd.print("!!");}
if (rxfreq > 440000000){lcd.print("!!");}

delay(deltime);

}
// and tx..
void showtx(){
  
lofreq = frequency(tunehigh, tunelow);

lcd.clear();
lcd.print("LO:");
lcd.print(lofreq);
lcd.print("Hz");
lcd.setCursor(0,1);

lcd.print("TX:");
lcd.print(lofreq);
lcd.print("Hz");
if (rxfreq < 433000000){lcd.print("!!");}
if (rxfreq > 440000000){lcd.print("!!");}

delay(deltime);
}

//fuction to write frequency data to mcp23017 - make sure tunelow and tunehigh have the right values !!

void writefreq(){
  
Wire.beginTransmission(mcp_address);
  Wire.write(GPIOA);      // address bank A
  Wire.write((byte)tunelow);  // value to send - the 8 low bits
  Wire.endTransmission();
  
  Wire.beginTransmission(mcp_address);
  Wire.write(GPIOB);    // address bank B
  Wire.write((byte)tunehigh);  // value to send - the 8 high bits
  Wire.endTransmission();
}

//transmit mode:
void txmode(){
  tune = tunehigh << 8 | tunelow; //conbine bytes to int
  
   tune = tune - 1712;
  //values determined to shift the LO 21.4mhz higher - cos thats what happens !
  //split int into 2 bytes 
  tunehigh = (byte)highByte(tune);
  tunelow = (byte)lowByte(tune);
 
  //write to mcp23027
  
  writefreq();
  
  //display output
  showtx();
  
  do
  {
    ptt = digitalRead(8);
    
  }
  while (ptt == LOW);
  
  tune = tune + 1712;
 
  tunehigh = (byte)highByte(tune);
  tunelow = (byte)lowByte(tune);
  writefreq();
  showrx();
}
 
 
#define I2CADDR 0x20 //for mcp23008 connected to keypad

const byte ROWS = 4; //four rows
const byte COLS = 3; //three columns
char keys[ROWS][COLS] = {
  {'1','2','3'},
  {'4','5','6'},
  {'7','8','9'},
  {'*','0','#'}
};
byte rowPins[ROWS] = {0, 1, 2, 3}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {4, 5, 6}; //connect to the column pinouts of the keypad

Keypad_MCP keypad = Keypad_MCP( makeKeymap(keys), rowPins, colPins, ROWS, COLS, I2CADDR );
char entered[10];
/*void entrymode(){
  //this will probably be the largest function. its for handling keypad entry after we've had one key press...
  
 for (int x = 0; 9 ; x++) {
  entered[x] = key;
  lcd.clear();
  lcd.print(entered);
 
 do {
   key = keypad.getKey(); }
   while (key == false);
   //when return is pressed convert entered digits to long int convert that to tuning word split in half and write to mcp23017 !
   
   writefreq();
   
   }
 }
}*/

void doEncoder() {
  /* If pinA and pinB are both high or both low, it is spinning
   * forward. If they're different, it's going backward.
   *
   * For more information on speeding up this process, see
   * [Reference/PortManipulation], specifically the PIND register.
   */
  
  tune = tunehigh << 8 | tunelow; //conbine bytes to int
  
  
 
  
  if (digitalRead(encoder0PinA) == digitalRead(encoder0PinB)) {
    tune++;
  } else {
    tune--;
  }
  //split int into 2 bytes 
  tunehigh = (byte)highByte(tune);
  tunelow = (byte)lowByte(tune);
  
   //write to mcp23027
  
  writefreq();
  
  //display output
  showrx(); 
  
}
void setup() {
  // put your setup code here, to run once:
  
  // rotary encoder stuff
  
  pinMode(encoder0PinA, INPUT); 
  //digitalWrite(encoder0PinA, HIGH);       // turn on pullup resistor - my cheap chinese ones hve them on the board.
  pinMode(encoder0PinB, INPUT); 
  //digitalWrite(encoder0PinB, HIGH);       // turn on pullup resistor 

  attachInterrupt(0, doEncoder, CHANGE);  // encoder pin on interrupt 0 - pin 2
  
  
  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);
  // Print a message to the LCD.
  lcd.print("Flexi VFO v0.0");
delay(1000);


//buttons for testing - now using encoder
pinMode(8, INPUT);
//pinMode(9, INPUT);
//pinMode(10, INPUT);
//digitalWrite(9, HIGH); //sets internal pullups so that you dont need external resistors
//digitalWrite(10, HIGH);//button pushes are detected by line gowing low.
digitalWrite(8, HIGH);

//I2C setup 
Wire.begin(); //creates a Wire object

keypad.begin( );


// set up I/O pins for mcp23017 ( set as addr 0x24 )

// IOCON.BANK defaults to 0 which is what we want.
  // So we are using Table 1-4 on page 9 of datasheet
  
  Wire.beginTransmission(mcp_address);
  Wire.write((byte)0x00); // IODIRA register
  Wire.write((byte)0x00); // set all of bank A to outputs
  Wire.write((byte)0x00); // set all of bank B to outputs 
  Wire.endTransmission();

showrx();

}

void loop() {
  // put your main code here, to run repeatedly:
//read buttons
//button1 = digitalRead(9);
//button2 = digitalRead(10);
ptt = digitalRead(8);
key = keypad.getKey();

//do stuff

/*if (key) {
  //some one pushed a key..... Ummmmm.. what now ?
  entrymode();
}*/

if (ptt == LOW) {txmode();}

//if (tunelow == B11111111) {if (button1 == LOW) {tunelow = tunelow + tunestep;} tunehigh = tunehigh + 1;}//catch rolls
//if (button1 == LOW) {tunelow = tunelow + tunestep;}
//if (tunelow == 0) {if (button2 == LOW) {tunelow = tunelow - tunestep;} tunehigh = tunehigh - 1;}//catch byte roll-arounds
//if (button2 == LOW) {tunelow = tunelow - tunestep;}

//im not going to look for tunehigh rolls as you'd never expect it

//send tuning bytes to mcp23017 @ 0x24

//writefreq();

//if (button1 == LOW) {showrx();} // only update display when a change happens
//if (button2 == LOW) {showrx();}
//delay put in show rouitines to make UI responsive
//delay(200);
//npt much here now as main vfo style tuningn is done with an interupt.
}
