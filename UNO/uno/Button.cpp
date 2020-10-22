#include "Arduino.h"
#include "Button.h"
#include "ControlBoxButton.h"

Button::Button(uint8_t pin, char ch, ControlBoxButton btn) : _buttonPin(pin), _command(ch), _btn(btn) { }

// Set pinMode for Button
void Button::begin() {
  pinMode(_buttonPin, INPUT);
}

// Function to check if Button is released
bool Button::isPressed() {
  int buttonState = digitalRead(_buttonPin);
  if (buttonState == HIGH) {
    return true;
  } else {
    return false;
  }
}

ControlBoxButton Button::getEnumType() {
  return _btn;
}

char Button::getCharCommand() {
  return _command;
}
