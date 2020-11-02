//  SerialIn_SerialOut_HM-10_01
//
//  Uses hardware serial to talk to the host computer and AltSoftSerial for communication with the bluetooth module
//
//  What ever is entered in the serial monitor is sent to the connected device
//  Anything received from the connected device is copied to the serial monitor
//  Does not send line endings to the HM-10
//
//  Pins
//  BT VCC to Arduino 5V out.
//  BT GND to GND
//  Arduino D8 (SS RX) - BT TX no need voltage divider
//  Arduino D9 (SS TX) - BT RX through a voltage divider (5v to 3.3v)
//
#include <AltSoftSerial.h>
#include "Parse.h"
#include "TrainStatus.h"
#include <LiquidCrystal_PCF8574.h>
#include <Wire.h>
#include "ControlBoxButton.h"
#include "Button.h"

/*
  LCM1602 Display with I2C module
*/
#define bpin_estop 1
#define bpin_changedir 2
#define bpin_startstop 3
#define bpin_doors 4


// ***** CUSTOM CHARACTER *****
int minusOne[8] = { 0b00010, 0b00110, 0b11010, 0b00010, 0b00111, 0b00000, 0b00000, 0b00000 };
int doubleAngledBrackets[8] = { 0b00000, 0b00000, 0b10100, 0b01010, 0b00101, 0b01010, 0b10100, 0b00000 };
int upArrow[8] = { 0b00100, 0b01110, 0b11111, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100 };
int downArrow[8] ={ 0b00100, 0b00100, 0b00100, 0b00100, 0b00100, 0b11111, 0b01110, 0b00100 };
// int myHeart[8] = {0, 10, 31, 31, 14, 4, 0, 0};

// ***** GLOBAL VARIABLES AND CONFIGS *****
// **** velocity display configs ****
const int v_lenIncDecimalPoint = 5;
const int v_numDigsAfterDecimal = 3;
char v_outStr[8];

// **** direction ****
//traindirection dir = unknown;

// **** scrolling text variables ****
int ls_startIndex = 16;
int ls_endIndex = 0;
int rs_endIndex  = -1;
int rs_startIndex = -1;

// **** buttons ****
Button b_estop(bpin_estop, 'x', estop);
Button b_changedir(bpin_changedir, 'c', changedir);
Button b_startstop(bpin_startstop, 's', togglestartstop);
Button b_doors(bpin_doors, 'd', toggledoors);
//

// **** LCD ****
LiquidCrystal_PCF8574 lcd(0x27); // Set the lcd address to 0x27 for a 16 chars and 2 line display
// Try I2C address of 0x3f or 0x20 if 0x27 does not work


AltSoftSerial BTserial;
// https://www.pjrc.com/teensy/td_libs_AltSoftSerial.html


boolean NL = true;
Parse parser = Parse();
TrainStatus trainStatus;

void setup() {
  while (!Serial);
  Serial.begin(9600);
  BTserial.begin(9600);
  Serial.println("BTserial started at 9600");
  setup_lcd();
}

void loop() {
  loop_lcd();
  considerReadingInput();
  if (Serial.available())
  {
    char c = ' ';
    c = Serial.read();

    // do not send line end characters to the HM-10
    if (c != 10 & c != 13 )
    {
      BTserial.write(c);
    }

    // Echo the user input to the main window.
    // If there is a new line print the ">" character.
    if (NL) {
      NL = false;
    }
    Serial.write(c);
    if (c == 10) {
      NL = true;
    }
  }
}

void considerReadingInput() {
  String input = "";
  while (BTserial.available()) {
    input = input + (char) BTserial.read();
    delay(100);
  }

  if (input.length() > 0) {
    trainStatus = parser.parse(input);
  }
}


void setup_lcd() {
  // Check and connect to LCD. 
  int error;
  
  Serial.println("lcd...");

  // Wait on Serial to be available on Uno
  while (!Serial);

  Serial.println("Checking for LCD...");
  Wire.begin();
  Wire.beginTransmission(0x27);
  error = Wire.endTransmission();
  Serial.print("Error: ");
  Serial.print(error);

  if (error == 0) {
    Serial.println(": LCD found.");
    lcd.begin(16, 2); // initialize the lcd (16 chars, 2 lines)
  } else {
    Serial.println(": LCD not found.");
  }

  // Create custom characters
  lcd.createChar(1, minusOne);
  lcd.createChar(2, doubleAngledBrackets);
  lcd.createChar(3, upArrow);
  lcd.createChar(4, downArrow);
  
  // Initalise buttons
  b_estop.begin();
  b_changedir.begin();
  b_startstop.begin();
  b_doors.begin();

  // Set configs for lcd
  lcd.setBacklight(255); // Set LCD's brightness 

  // Show start up message
  lcd.setCursor(0, 0);
  lcd.print("Starting up");
  lcd.setCursor(0, 1);
  lcd.print("Hello all!");
  for (int i = 11; i < 16; i++) {
    lcd.setCursor(i, 0);
    lcd.print(".");
    delay(500);
  }
  delay(500);
  lcd.clear();
}

void loop_lcd() {
  // First line
  lcd.setCursor(0, 0);
  lcd.print(scrollLeft("Demo: " + String("\2") + "~ Scrolling single row only ~" + String("\3\4\2") + "  The quick brown fox jumps over the lazy dog"));
  
  // Second line
  displayVelocity(trainStatus.speed.toDouble());
  displayUnit();
  displayDirection(trainStatus.dir);
  delay(500);
}

ControlBoxButton randomiseButtonPressed() {
  int i = rand() % 4;
  ControlBoxButton btn;

  if (i < 0) {
    btn = estop;
  } else if (i < 1) {
    btn = changedir;
  } else if (i < 2) {
    btn = toggledoors;
  } else {
    btn = togglestartstop;
  }
  return btn;
}

float randomiseFloat() {
  // Train should be travelling between 0.1 - 0.4 ms-1
  return (float)rand() / 1000;
}

String buttonToString(ControlBoxButton btn) {
  if (btn == estop) {
    return "Emergency Stop";
  } else if (btn == changedir) {
    return "Change Direction";
  } else if (btn == toggledoors) {
    return "Opening/Closing Doors";
  } else {
    return "Start/Stop Train";
  }
}

// **** print to LCD funcs ****
String displayButtonPressed() {
  lcd.setCursor(0, 0); // First line
  String btnPressed = buttonToString(randomiseButtonPressed());
  lcd.print(btnPressed);
  return btnPressed;
}

void displayVelocity(double speed) {
  lcd.setCursor(0, 1);
  lcd.write(byte(2));
  dtostrf(speed, v_lenIncDecimalPoint, v_numDigsAfterDecimal, v_outStr);
  lcd.print(v_outStr);
}

void displayUnit() {
  // Print unit "m/s" or "ms-1" custom char
  lcd.setCursor(7, 1);
  lcd.print(" ms");
  lcd.setCursor(10, 1);
  lcd.write(byte(1)); // Write minusOne character to lcd
}

void displayDirection(String dir) {
  lcd.setCursor(12, 1);
  lcd.print(dir);
}

String scrollLeft(String text) {
  String str;
  String strPadded = "                " + text + "                "; //16 whitespace paddings + text + 16 whitespace paddings

  // Extract whatever is needed for display and increment startIndex and endIndex by one
  str = strPadded.substring(ls_startIndex++, ls_endIndex++);

  // When index is out of bound, reset indexes to default positions
  if (ls_startIndex > strPadded.length()) {
    resetLeftScrollIndexes();
  }
  return str;
}

// Use when a new different text is being printed or when indexes are out of bound
void resetLeftScrollIndexes() {
  ls_startIndex = 16;
  ls_endIndex = 0;
}

String scrollRight(String text) {
  String str;
  String strPadded = "                " + text + "                "; //16 whitespace paddings + text + 16 whitespace paddings
  // When index is out of bound, reset indexes to default positions
  if (rs_endIndex  < 1) {
    rs_endIndex  = strPadded.length();
    rs_startIndex = rs_endIndex  - 16;
  }
  // Extract whatever is needed for display and increment startIndex and endIndex by one
  str = strPadded.substring(rs_startIndex--, rs_endIndex--);

  return str;
}

void resetRightScrollIndexes() {
  rs_endIndex  = -1;
  rs_startIndex = -1;
}
