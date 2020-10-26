#ifndef Button_h
#define Button_h
#include "ControlBoxButton.h"

class Button {
  private:
    uint8_t _buttonPin;
    char _command;
    String _btn;

  public:
    Button(uint8_t pin, char ch, String btn);
    void begin();
    bool isPressed();
    String getButtonName();
    char getCommand();
};
#endif
