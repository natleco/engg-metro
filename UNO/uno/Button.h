#ifndef Button_h
#define Button_h
#include "ControlBoxButton.h"

class Button {
  private:
    uint8_t _buttonPin;
    char _command;
    ControlBoxButton _btn;

  public:
    Button(uint8_t pin, char ch, ControlBoxButton btn);
    void begin();
    bool isPressed();
    ControlBoxButton getEnumType();
    char getCharCommand();
};
#endif
