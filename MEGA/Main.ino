/*
  State Machine, Motors & Sensors - Arduino MEGA (T1/Carriage 2)
  Team: T1_C3
  Members: Afroja Rowson Akter, Nicholas Drummond, Safal Maharjan, Jayakrithi Shivakumar & Nathan Lecompte
*/

#define DEBUG 0
#define ENABLE_SENSORS 1
#define ENABLE_MOTORS 1
#define ENABLE_COMMS 1
#define ENABLE_DOORS 1

#define BAUD_RATE 9600

#define MOTOR_PIN 8
#define MOTOR_DRIVER_PIN 9
#define ENCODER_PIN_A 2
#define ENCODER_PIN_B 3

class State {
  public:
    /*
      Status codes:
      0 = Default state (no errors)
      1 = Emergency stopped state
      2 = Comms calibration failed state
      3 = Comms link disconnected state
      4 = Received malformed command over Comms link state
      5 = Failed to send command over Comms link state
      6 = Motors calibration failed state
      7 = Motors command failed state
      8 = Sensors calibration failed state
      9 = Sensors received malformed state
    */
    char status = 0;

    // Direction of train (0 or 1)
    char direction = 0;

    // Speed of train (1500 = stop, < 1500 = reverse, > 1500 = forward)
    unsigned long speed = 1500;

    #if ENABLE_DOORS
      // Flag if doors are open
      char doorsOpen = 0;
    #endif
};

State state;

#if ENABLE_COMMS
  class Comms {
    private: 
      char command;

    public:
      Comms() {
        Serial.begin(BAUD_RATE);
      }

      /*
        Send test command over comms and check that response is correct
      */
      bool calibrate() {
        char response;
        Serial.print("C:1");
        delay(100);
        if (Serial.available() != 0) {
          response = Serial.read();
          return (response == 'b');
        }
        return false;
      }

      /*
        Data is sent in this format: 
          < status code (0-9) >:< data type (a, d, s) >_< data >
      */
      bool sendCommand(char type, int data) {
        char message[10];
        if (type && data) {
          snprintf(message, sizeof message, "%s:%s_%s", state.status, type, data);
          Serial.print("<");
          delay(100);
          Serial.print(message);
          delay(100);
          Serial.print(">");
          return true;
        } else {
          return false;
        }
      }

      /*
        Data is received in this format:
          x = Emergency stop
          g = Start train
          s = Stop train
          c = Change train direction
          d = Open/close doors
      */
      char receivedCommand() {
        char response;
        while (Serial.available()) {
          response = Serial.read();
          if (response != '<' && response != '>') {
            command = response;
            delay(100);
          }
          if (response == '>') {
            break;
          }
        }
        return command;
      }
  };
#endif

#if ENABLE_MOTORS
  #include <Servo.h>
  #include <Encoder.h>

  class Motors {
    private:
      Servo motor;
      Encoder motorEncoder(ENCODER_PIN_A, ENCODER_PIN_B);
      char motorType = 0; // 0 = Train motors, 1 = Door motors

    public:
      Motors() {
        pinMode(MOTOR_PIN, OUTPUT); 
        motor.attach(MOTOR_PIN);
      }

      void setTrainAcceleration(char acceleration) {
        switch (acceleration) {
          case 0:
            if (::state.speed != 1500) {
              // Emergency stop (hard break)
              motor.writeMicroseconds(1500);
            }
            break;
          case 1:
            if (::state.speed != 1500) {
              // Stop train (gradual speed decrease)
              motor.writeMicroseconds(1500);
            }
            break;
          case 2:
            if (::state.speed != 1650) {
              // Slow train speed
              motor.writeMicroseconds(1650);
            }
            break;
          case 3:
            if (::state.speed != 1750) {
              // Medium train speed
              motor.writeMicroseconds(1750);
            }
            break;
          case 4:
            if (::state.speed != 2000) {
              // Fast train speed
              motor.writeMicroseconds(2000);
            }
            break;
        }
      }

      void changeTrainDirection(char direction) {
        switch (direction) {
          case 0:
            if (motorType == 0 && ::state.direction == 1) {
              // Forwards
              ::state.direction = 0;
            }
            break;
          case 1:
            if (motorType == 0 && ::state.direction == 0) {
              // Backwards
              ::state.direction = 1;
            }
            break;
        }
      }

      #if ENABLE_DOORS
        void openTrainDoors() {
          if (motorType == 1 && ::state.doorsOpen == 0) {
            ::state.doorsOpen = 1;
          }
        }

        void closeTrainDoors() {
          if (motorType == 1 && ::state.doorsOpen == 1) {
            ::state.doorsOpen = 0;
          }
        }
      #endif
  };
#endif

#if ENABLE_SENSORS
  class Sensors {
    public:
      Sensors() {
        // Use ::state to access the state class in this scope
      }

      /*
        Check color sensors sense correct colors, and other stuff
      */
      void calibrate() {
        
      }

      void receivedCommand() {

      }
  };
#endif

Comms comms;
Motors motors;
Sensors sensors;

void setup() {
  #if DEBUG
    Serial1.begin(9600);
    Serial1.print("Entered DEBUG mode\n");
  #endif

  Serial.begin(9600);
}

void loop() {
  switch (comms.receivedCommand()) {
    case 'x':
      break;
    
    case 'g':
      break;
    
    case 's':
      break;
    
    case 'c':
      break;

    case 'd':
      break;
  }
}
