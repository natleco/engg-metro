#include "Arduino.h"
#include "Button.h"
#include "ControlBoxButton.h"

Button::Button(uint8_t pin, char ch, String btn) : _buttonPin(pin), _command(ch), _btn(btn) { begin(); }
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

String Button::getButtonName() {
  return _btn;
}

char Button::getCommand() {
  return _command;
}
