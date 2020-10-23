// Button pins
#define bpin_estop 1
#define bpin_changedir 2
#define bpin_startstop 3
#define bpin_doors 4

const String bname_estop = "Emergency Stop";
const String bname_changedir = "Change Direction";
const String bname_startstop = "Start/Stop Train";
const String bname_doors = "Opening/Closing Doors";

// Button class
class Button {
  private:
    uint8_t _buttonPin;
    char _command;
    String  _btnname;

  public:
    Button(uint8_t pin, char ch, String btnname) : _buttonPin(pin), _command(ch), _btnname(btnname) {}

    // Set pinMode for Button
    void begin() {
      pinMode(_buttonPin, INPUT);
    }

    bool isPressed() {
      int buttonState = digitalRead(_buttonPin);
      if (buttonState == HIGH) {
        return true;
      } else {
        return false;
      }
    }
    
    String getButtonName() {
      return _btnname;
    }

    char getCommand() {
      return _command;
    }
};

// **** buttons ****
Button b_estop(bpin_estop, 'x',  bname_estop);
Button b_changedir(bpin_changedir, 'c', bname_changedir);
Button b_startstop(bpin_startstop, 's', bname_startstop);
Button b_doors(bpin_doors, 'd', bname_doors);

Button btns[] = {b_estop, b_changedir, b_startstop, b_doors};

void setup() {
  // Initalise buttons
  b_estop.isPressed();
  b_changedir.isPressed();
  b_startstop.isPressed();
  b_doors.isPressed();
}

void loop() {
  checkButtonPresses(btns);

}

int checkButtonPresses(Button btn[]){
  int n = 0;
  if(b_estop.isPressed()){
    n++;
  }
  b_changedir.isPressed();
  b_startstop.isPressed();
  b_doors.isPressed();
  return n;
}
