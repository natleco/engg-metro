/*
  LCM1602 Display with I2C module
*/
#define bpin_estop 1
#define bpin_changedir 2
#define bpin_startstop 3
#define bpin_doors 4


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

int doubleAngledBrackets[8] =
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

int upArrow[8] =
{
  0b00100,
  0b01110,
  0b11111,
  0b00100,
  0b00100,
  0b00100,
  0b00100,
  0b00100
};

int downArrow[8] =
{
  0b00100,
  0b00100,
  0b00100,
  0b00100,
  0b00100,
  0b11111,
  0b01110,
  0b00100
};
// int myHeart[8] = {0, 10, 31, 31, 14, 4, 0, 0};

class Button {
  private:
    uint8_t _buttonPin;
    char _command;
    button _btn;

  public:
    Button(uint8_t pin, char ch, button btn) : _buttonPin(pin), _command(ch), _btn(btn) {}

    // Set pinMode for Button
    void begin() {
      pinMode(_buttonPin, INPUT);
    }

    // Function to check if Button is released
    bool isPressed() {
      int buttonState = digitalRead(_buttonPin);
      if (buttonState == HIGH) {
        return true;
      } else {
        return false;
      }
    }

    button getEnumType() {
      return _btn;
    }

    char getCharCommand() {
      return _command;
    }
 
};

// ***** GLOBAL VARIABLES AND CONFIGS *****
// **** velocity display configs ****
const int v_lenIncDecimalPoint = 5;
const int v_numDigsAfterDecimal = 3;
char v_outStr[8];

// **** direction ****
traindirection dir = unknown;

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


void setup() {
  // Check and connect to LCD. 
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

void loop() {
  // First line
  lcd.setCursor(0, 0);
  //displayCustomCharsOne();
  //scrollLeft(buttonToString(randomiseButtonPressed()));
  lcd.print(scrollLeft("Demo: " + String("\2") + "~ Scrolling single row only ~" + String("\3\4\2") + "  The quick brown fox jumps over the lazy dog"));
  
  
  // Second line
  displayVelocity();
  displayUnit();
  displayDirection();
  delay(500);
}


// ***** FUNCTIONS *****
// **** randomisor for testing ****
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

// **** type converter ****
String trainDirToString(traindirection dir) {
  if (dir == west) {
    return "West";
  } else if (dir == east) {
    return "East";
  } else {
    return "Unkn";
  }
}

String buttonToString(button btn) {
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

void displayVelocity() {
  lcd.setCursor(0, 1);
  lcd.write(byte(2)); // Write doubleAngledBrackets character to lcd
  //dtostrf(velocity, v_lenIncDecimalPoint, v_numDigsAfterDecimal, v_outStr); // Format the velocity
  dtostrf(randomiseFloat(), v_lenIncDecimalPoint, v_numDigsAfterDecimal, v_outStr); 
  lcd.print(v_outStr);
}

void displayUnit() {
  // Print unit "m/s" or "ms-1" custom char
  lcd.setCursor(7, 1);
  lcd.print(" ms");
  lcd.setCursor(10, 1);
  lcd.write(byte(1)); // Write minusOne character to lcd
}

void displayDirection() {
  lcd.setCursor(12, 1);
  lcd.print(trainDirToString(randomiseDirection()));
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
