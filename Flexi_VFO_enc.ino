//    Code for a flexible VFO for diode matrix tuned radios.. see README
//    Copyright (C) 2016  Simon Newhouse M0NWY
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.

//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program.  If not, see <https://www.gnu.org/licenses/>.


//Includes
#include <LiquidCrystal.h>
#include <Wire.h>

volatile boolean TurnDetected;
volatile boolean up;

const int PinCLK=2;                   // Used for generating interrupts using CLK signal
const int PinDT=9;                    // Used for reading DT signal

// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(12, 11, 5, 4, 3, 7);

//set frequancy bytes up - tuning takes 2 bytes 
//currently set at 411.6 mhz for a receve frerquency of 433.0mhz
//i've got it a bit backwards but a zero represents a disconnection of the pin and a 1 is a shoted pin on the diode matrix
//this works for driving the fets that'll be doing the matrix switching but I have to "not" the bits to get the frequency readout right.
//it also means that i constantly get things backwards !!

byte tunehigh = B01111111;
 byte tunelow = B01011111;
int tune = tunehigh << 8 | tunelow;
//init other vars
const int deltime = 0;
long lofreq;
long rxfreq;
int button1;
int button2;
int tunestep = 1;
int ptt;
char key;
const byte  mcp_address=0x24;      // I2C Address of MCP23017 Chip - not the default address - connect pins or adjust code accordingly
const byte  GPIOA=0x12;            // Register Address of Port A
const byte  GPIOB=0x13;            // Register Address of Port B
int shift = 0; //repeater shift flag

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
//512+64+32
//function to refresh display for RX

void showrx(){
  
lofreq = frequency(tunehigh, tunelow);

lcd.clear();
lcd.print("LO:");
lcd.print(lofreq);
lcd.print("Hz");
lcd.print(" ");
lcd.print(shift);
lcd.setCursor(0,1);

rxfreq = lofreq + 21400000;

lcd.print("RX:");
lcd.print(rxfreq);
lcd.print("Hz");

if (rxfreq < 430000000){lcd.print("!!");}// out of band exclamtion marks
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
if (lofreq < 430000000){lcd.print("!!");}
if (lofreq > 440000000){lcd.print("!!");}

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
  tune = tunehigh << 8 | tunelow; //combine bytes to int
  
   if ( shift == 0 ) { tune = tune - 1712; }
   if ( shift == 1 ) { tune = tune - 1840; }
   if ( shift == 2 ) { tune = tune - 2320; }
   
  //values determined to shift the LO 21.4mhz higher - cos the reciver runs at 21.4mhz lower than the transmitter in this radio !
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
  
   if ( shift == 0 ) { tune = tune + 1712; }
   if ( shift == 1 ) { tune = tune + 1840; }
   if ( shift == 2 ) { tune = tune + 2320; }
 
  tunehigh = (byte)highByte(tune);
  tunelow = (byte)lowByte(tune);
  writefreq();
  showrx();
}
 

void isr ()  {     // Interrupt service routine is executed when a transition is detected on CLK
 if (digitalRead(PinCLK))
   up = digitalRead(PinDT);
 else
   up = !digitalRead(PinDT);

 
 TurnDetected = true;
 
}

void adjustshift(){
  if (shift == 0 ) { shift = 1; }
  else if (shift == 1 ) { shift = 2; }
  else if (shift == 2 ) { shift = 0; }
  showrx();
  delay(500);
}

void setup() {
  // put your setup code here, to run once:
 
  // rotary encoder stuff

  pinMode(PinCLK,INPUT);
 pinMode(PinDT,INPUT);  
// pinMode(PinSW,INPUT);
attachInterrupt (0,isr,CHANGE);   // interrupt 0 is always connected to pin 2 on Arduino UNO


  
  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);
  // Print a message to the LCD.
  lcd.print("    M0NWY");
  lcd.setCursor(0,1);
  lcd.print("  Presents....");
  delay(2000);
  lcd.clear();
  lcd.print("Flexi VFO v0.1");
  lcd.setCursor(0,1);
  lcd.print("For TAIT 555 TRX");
  delay(2000);


//buttons
pinMode(8, INPUT);
pinMode(10, INPUT);
digitalWrite(8, HIGH);  //set internal pullups - button push is detected by pin going low
digitalWrite(10, HIGH);  //not actually recomended but it works..

//I2C setup 
Wire.begin(); //creates a Wire object

// set up I/O pins for mcp23017 ( set as addr 0x24 )

// IOCON.BANK defaults to 0 which is what we want.
  // So we are using Table 1-4 on page 9 of datasheet
  
  Wire.beginTransmission(mcp_address);
  Wire.write((byte)0x00); // IODIRA register
  Wire.write((byte)0x00); // set all of bank A to outputs
  Wire.write((byte)0x00); // set all of bank B to outputs 
  Wire.endTransmission();
writefreq();
showrx();

}

void loop() {
  // put your main code here, to run repeatedly:


//read buttons
button1 = digitalRead(10);
ptt = digitalRead(8);

//do stuff

if (ptt == LOW) {txmode();}
if (button1 == LOW) {adjustshift();}
if (TurnDetected)  {		   // do this only if rotation was detected
   if (up) {
     tune = tunehigh << 8 | tunelow;  // the tuning word is split into 2 bytes.. this could all probably be done with an int but all the examples for writing to the mcp23017 treat the 8 bit ports as separate so I keep them as separate bytes.
     tune++;
     tunehigh = (byte)highByte(tune);
     tunelow = (byte)lowByte(tune);
     writefreq();
     showrx(); }
   else {
     tune = tunehigh << 8 | tunelow;
     tune--;
     tunehigh = (byte)highByte(tune);
     tunelow = (byte)lowByte(tune);
     writefreq();
     showrx(); }
   TurnDetected = false;          // do NOT repeat IF loop until new rotation detected
}

}
