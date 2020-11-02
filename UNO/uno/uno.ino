//
//  Control Box - Arduino Uno (Train 2)
//  Team: T1_C5
//  Members: Hong Lim, Mingze Song
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
#include "Button.h"

// ***** GLOBAL VARIABLES AND CONFIGS *****
// **** button pins ****
#define bpin_estop 3
#define bpin_changedir 4
#define bpin_startstop 2
#define bpin_doors 5

// **** button names ****
const String bname_estop = "Emergency Stop";
const String bname_changedir = "Change Direction";
const String bname_startstop = "Start/Stop Train";
const String bname_doors = "Opening/Closing Doors";

// **** buttons ****
Button b_estop(bpin_estop, 'x',  bname_estop);
Button b_changedir(bpin_changedir, 'c', bname_changedir);
Button b_startstop(bpin_startstop, 's', bname_startstop);
Button b_doors(bpin_doors, 'd', bname_doors);

// ***** CUSTOM CHARACTER *****
int minusOne[8] = { 0b00010, 0b00110, 0b11010, 0b00010, 0b00111, 0b00000, 0b00000, 0b00000 };
int doubleAngledBrackets[8] = { 0b00000, 0b00000, 0b10100, 0b01010, 0b00101, 0b01010, 0b10100, 0b00000 };
int upArrow[8] = { 0b00100, 0b01110, 0b11111, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100 };
int downArrow[8] = { 0b00100, 0b00100, 0b00100, 0b00100, 0b00100, 0b11111, 0b01110, 0b00100 };
int myHeart[8] = {0, 10, 31, 31, 14, 4, 0, 0};

// **** velocity display configs ****
const int v_lenIncDecimalPoint = 5;
const int v_numDigsAfterDecimal = 3;
char v_outStr[8];

// **** scrolling text variables ****
int ls_startIndex = 16;
int ls_endIndex = 0;
int rs_endIndex  = -1;
int rs_startIndex = -1;

// **** LCD ****
LiquidCrystal_PCF8574 lcd(0x27);

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
      Serial.write(c);
    }

    // Echo the user input to the main window.
    // If there is a new line print the ">" character.
    if (NL) {
      NL = false;
    }
    //    Serial.print(c);
    if (c == 10) {
      NL = true;
    }
  }
  listenForButtonClicks();
}

Button btnPressed = Button(-1, "_", "");
bool isButtonPressed = false;
void listenForButtonClicks() {
  if (b_doors.isPressed()) {
    resetLeftScrollIndexes();
    Serial.println(b_doors.getButtonName());
    btnPressed = b_doors;
    isButtonPressed = true;
  }
  if (b_changedir.isPressed()) {
    resetLeftScrollIndexes();
    Serial.println(b_changedir.getButtonName());
    btnPressed = b_changedir;
    isButtonPressed = true;
  }
  if (b_startstop.isPressed()) {
    resetLeftScrollIndexes();
    Serial.println(b_startstop.getButtonName());
    btnPressed = b_startstop;
    isButtonPressed = true;
  }
  if (b_estop.isPressed()) {
    resetLeftScrollIndexes();
    Serial.println(b_estop.getButtonName());
    btnPressed = b_estop;
    isButtonPressed = true;
  }
  lcd.setCursor(0, 0);
  // send button.getCommand() to mega
  sendCommand(btnPressed.getCommand());
  lcd.print(scrollLeft(btnPressed.getButtonName()));
  isButtonPressed = false;

}

void sendCommand(char command) {
  if (isButtonPressed) {
    Serial.println(command);
    BTserial.print('<');
    BTserial.print(command);
    BTserial.print('>');
  }
}

void considerReadingInput() {
  String input = "";
  while (BTserial.available()) {
    input = input + (char) BTserial.read();
    delay(70);
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
  lcd.createChar(5, myHeart);

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
  }
  lcd.clear();
}

void loop_lcd() {
  // First line
  lcd.setCursor(0, 0);
  //  lcd.print(scrollLeft("Demo: " + String("\2") + "~ Scrolling single row only ~" + String("\3\4\2") + "  The quick brown fox jumps over the lazy dog"));
  // Second line
  displayVelocity(trainStatus.speed.toDouble());
  displayUnit();
  displayDirection(trainStatus.dir);
  delay(300);
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
  lcd.print("ms");
  lcd.setCursor(9, 1);
  lcd.write(byte(1)); // Write minusOne character to lcd
  lcd.write(byte(5));
}

void displayDirection(String dir) {
  lcd.setCursor(12, 1);
  lcd.print(dir);
}

String scrollLeft(String text) {
  String str;
  String strPadded = "              " + text + "               "; //16 whitespace paddings + text + 16 whitespace paddings

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
