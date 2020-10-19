/*
  LCM1602 Display with I2C module
*/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <LiquidCrystal_PCF8574.h>
#include <Wire.h>


enum traindirection {
  unknown,
  east,
  west
};

enum button {
  estop,
  changedir,
  toggledoors,
  togglestartstop
};

// ***** CUSTOM CHARACTER *****
int heart[8] = { // needs to convert to int[] to make it work
  B00000,
  B01010,
  B11111,
  B11111,
  B01110,
  B00100,
  B00000,
  B00000
};

int minusOne[8] =
{
  0b00010,
  0b00110,
  0b11010,
  0b00010,
  0b00111,
  0b00000,
  0b00000,
  0b00000
};

int smiley[8] =
{
  0b00000,
  0b01010,
  0b01010,
  0b00000,
  0b10001,
  0b01110,
  0b00000,
  0b00000
};

int c1[8] =
{
  0b01010,
  0b10101,
  0b01010,
  0b10101,
  0b01010,
  0b10101,
  0b01010,
  0b10101
};

int c2[8] =
{
  0b10101,
  0b01010,
  0b10101,
  0b01010,
  0b10101,
  0b01010,
  0b10101,
  0b01010
};

int dd[8] =
{
  0b00000,
  0b00000,
  0b10100,
  0b01010,
  0b00101,
  0b01010,
  0b10100,
  0b00000
};

int up[8] = 
{
0b00100,
0b01110,
0b11111,
0b00100,
0b00100,
0b00100,
0b00100,
0b00100
}; // TODO: charmap for up arrow
int down[8] = 
{
0b00100,
0b00100,
0b00100,
0b00100,
0b00100,
0b11111,
0b01110,
0b00100
}; // TODO: charmap for down arrow
// int myHeart[8] = {0, 10, 31, 31, 14, 4, 0, 0};

// ***** GLOBAL VARIABLES AND CONFIGS *****
// **** velocity display configs ****
const int v_lenIncDecimalPoint = 5;
const int v_numDigsAfterDecimal = 3;
char v_outStr[8];

// **** direction ****
traindirection dir = unknown;

// **** gif toggle ****
int c = 1;

LiquidCrystal_PCF8574 lcd(0x27); // Set the lcd address to 0x27 for a 16 chars and 2 line display
// LiquidCrystal_PCF8574 LCD(0x27, 16, 2);
// Try I2C address of 0x3f or 0x20 if 0x27 does not work


void setup() {
  // Connect to LCD
  int error;

  Serial.begin(115200);
  Serial.println("lcd...");

  // Wait on Serial to be available on Uno
  while (!Serial)
    ;

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
  lcd.createChar(0, heart);
  lcd.createChar(1, minusOne);
  lcd.createChar(2, smiley);
  lcd.createChar(3, c1);
  lcd.createChar(4, c2);
  lcd.createChar(5, dd);
  lcd.createChar(6, down);
  lcd.createChar(7, up);

  // Set configs for buttons

  // Set configs for lcd
  lcd.setBacklight(255);

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

void loop() {
  // Check button press
  // Check response from mega

  // First line
  //char* btnPressed = displayButtonPressed();

  
  lcd.setCursor(0, 0);
  lcd.print("btn:");
  // Testing custom characters
  lcd.write(byte(0));
  lcd.write(byte(5));
  lcd.write(byte(2));
  lcd.write(byte(c + 3));
  lcd.write(byte(4 - c));
  lcd.write(byte(c + 6));
  lcd.write(byte(7 - c));
   
  
  // Second line
  displayVelocity();
  displayUnit();
  displayDirection();
  delay(500);

  // scroll first line
  //scrollToLeft(btnPressed, 7, 0);
  c = (c + 1) % 2;
}


// ***** FUNCTIONS *****
// **** randomisor for testing ****
const char* randomiseDirectionOLD() {
  int i = rand() % 11;
  char *d = "****";

  if (i < 5) {
    dir = east;
    strcpy(d, "East");
  } else if (i < 10) {
    dir = west;
    strcpy(d, "West");
  } else {
    dir = unknown;
    strcpy(d, "Unkn");
  }

  return d;
}

traindirection randomiseDirection() {
  int i = rand() % 12;
  traindirection dir;
  
  if (i < 5) {
    dir = east;
  } else if (i < 10) {
    dir = west;
  } else {
    dir = unknown;
  }
  return dir;
}

button randomiseButtonPressed() {
  int i = rand() % 4;
  button btn;
  
  if (i < 0) {
    btn = estop;
  } else if (i < 1) {
    btn = changedir;
  } else if (i < 2){
    btn = toggledoors;
  } else{
    btn = togglestartstop;
  }
  return btn;
}


float randomiseFloat() {
  // Train should be travelling between 0.1 - 0.4 ms-1
  return (float)rand() / 1000;
}

// **** type converter ****
const char* trainDirToString(traindirection dir) {
  if (dir == west) {
    return "West";
  } else if (dir == east) {
    return "East";
  } else {
    return "Unkn";
  }
}

const char* buttonToString(button btn) {
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
const char* displayButtonPressed(){
  lcd.setCursor(0, 0); // First line
  char* btnPressed = "button_pressed:1234";
  
  btnPressed = buttonToString(randomiseButtonPressed());
  lcd.print(btnPressed);
  return btnPressed;
}

void displayVelocity() {
  lcd.setCursor(0, 1); // Second line
  lcd.write(byte(5));
  //dtostrf(velocity, v_lenIncDecimalPoint, v_numDigsAfterDecimal, v_outStr); // Format the velocity
  dtostrf(randomiseFloat(), v_lenIncDecimalPoint, v_numDigsAfterDecimal, v_outStr);
  lcd.print(v_outStr);

}

void displayUnit() {
  // Print unit "m/s" or "ms-1" custom char
  lcd.setCursor(7, 1);
  //lcd.print(" m/s");
  lcd.print(" ms");
  lcd.setCursor(10, 1);
  lcd.write(byte(1)); //character minusOne
}

void displayDirection() {
  lcd.setCursor(12, 1);
  lcd.print(trainDirToString(randomiseDirection()));
  //lcd.print(randomiseDirection());
}

void scrollToLeft(char* text, int positions, int row){
  char* txtPntr = text;
 
  
  for (int positionCounter = 0; positionCounter < positions; positionCounter++, txtPntr++) {
    lcd.setCursor(0, row);
    
    // scroll one position left:
    lcd.print(txtPntr);
    lcd.print("         ");
    // wait a bit:
    delay(250);
  }
}
