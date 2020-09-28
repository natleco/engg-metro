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

#define MAX_COMMANDS 100

#define MOTOR_PIN 8
#define MOTOR_DRIVER_PIN 9
#define ENCODER_PIN_A 2
#define ENCODER_PIN_B 3

#define RGB_COLOROUT 8
#define RGB_S0 4
#define RGB_S1 5 
#define RGB_S2 7
#define RGB_S3 6

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

    char direction = 0;

    /*
      Speed of train (1500 = stop, < 1500 = reverse, > 1500 = forward)
    */
    unsigned long speed = 1500;

    #if ENABLE_DOORS
      char doorsOpen = 0;
    #endif

    /*
      Queue of commands received from either Sensors or Comms
    */
    char commandQueue[MAX_COMMANDS];
    int commandQueueBack = -1;
    int commandQueueFront = 0;
    int commandQueueCount = 0;

    /*
      Adds a command to the commandQueue
    */
    void enqueueCommand(char command) {
      if (commandQueueCount != MAX_COMMANDS) {
        if (commandQueueBack == MAX_COMMANDS - 1) {
          commandQueueBack = -1;            
        }
        commandQueue[++commandQueueBack] = command;
        commandQueueCount++;
      }
    }

    /*
      Removes oldest command from the commandQueue
    */
    char dequeueCommand() {
      char command = commandQueue[commandQueueFront++];
      if (commandQueueFront == MAX_COMMANDS) {
        commandQueueFront = 0;
      }
      commandQueueCount--;
      return command;
    }

};

State state;

#if ENABLE_COMMS
  class Comms {
    public:
      #if DEBUG
        void calibrate() {
          Serial.print("Begin calibration for COMMS...");
          char response;
          Serial1.print("C:1");
          delay(1000);
          if (Serial1.available() != 0) {
            response = Serial1.read();
            if (response == 'b') {
              Serial.print("COMMS calibration SUCCESS!");
            } else {
              Serial.print("COMMS calibration FAILED!");
            }
          }
        }
      #endif

      /*
        Data is sent in this format: 
          < status code (0-9) >:< data type (a, d, s) >_< data >
      */
      bool sendCommand(char type, int data) {
        char message[10];
        if (type && data) {
          snprintf(message, sizeof message, "%s:%s_%s", state.status, type, data);
          Serial1.print("<");
          delay(100);
          Serial1.print(message);
          delay(100);
          Serial1.print(">");
          return true;
        } else {
          return false;
        }
      }

      /*
        Data is received in this format:
          x = Emergency stop
          m = Start train
          s = Stop train
          c = Change train direction
          d = Open/close doors
      */
      char receivedCommand() {
        char response;
        char command = 'n';
        while (Serial1.available()) {
          response = Serial1.read();
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

  /* 
    Motor Type: Servo with built-in rotary encoder
    Motor Model: NeveRest Classic 60 Gearmotor
    Motor Driver: Spark Motor Controller
    Number of Motors: 1
  */
  class Motors {
    private:
      Servo motor;
      char motorType = 0; // 0 = Train motors, 1 = Door motors
      Encoder encoder = Encoder(ENCODER_PIN_A, ENCODER_PIN_B);

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
  #include <EEPROM.h>

  /* 
    Sensor Type: RGB sensors
    Sensor Model: XC3708
    Number of Sensors: 1
  */
  class Sensors {
    private:
      // Reads the Frequency of the respective colours 
      int redFrequency = 0;
      int greenFrequency = 0;
      int blueFrequency = 0;

      // Amount of primary colour detection from the sensed object
      int redColor;
      int greenColor;
      int blueColor;
      
      // Minimum and Maximum values for calibration
      typedef struct Range {
        int redMin;
        int redMax;
        int greenMin;
        int greenMax;
        int blueMin;
        int blueMax;
      } Range;
      Range colorRange;

    public:
      char color = 'n';

      Sensors() {
        // Get color range from EEPROM memory
        EEPROM.get(0, colorRange);

        pinMode(RGB_S0, OUTPUT);
        pinMode(RGB_S1, OUTPUT);
        pinMode(RGB_S2, OUTPUT);
        pinMode(RGB_S3, OUTPUT);
        pinMode(13, OUTPUT); // TODO: Change for MEGA; only Tested for UNO
        pinMode(RGB_COLOROUT, INPUT);
      }

      #if DEBUG
        void calibrate() {
          Serial.print("Begin calibration for SENSORS...");

          // Aiming at WHITE color
          Serial.println("SENSORS Calibrating - Min range...");
          Serial.println("Detected color: WHITE");

          // Setting calibration values - Min range
          digitalWrite(13, HIGH);
          delay(2000);
          digitalWrite(RGB_S2, LOW);
          digitalWrite(RGB_S3, LOW);
          colorRange.redMin = pulseIn(RGB_COLOROUT, LOW);
          delay(100);

          digitalWrite(RGB_S2, HIGH);
          digitalWrite(RGB_S3, HIGH);
          colorRange.greenMin = pulseIn(RGB_COLOROUT, LOW);
          delay(100);

          digitalWrite(RGB_S2, LOW);
          digitalWrite(RGB_S3, HIGH);
          colorRange.blueMin = pulseIn(RGB_COLOROUT, LOW);
          delay(100);

          // Aiming at BLACK color
          Serial.println("SENSORS Calibrating - Max range...");
          digitalWrite(13, LOW);
          delay(2000);
          Serial.println("Detected color: BLACK");

          // Setting calibration values - Max range
          digitalWrite(13, LOW);
          delay(2000);
          digitalWrite(RGB_S2, LOW);
          digitalWrite(RGB_S3, LOW);
          colorRange.redMax = pulseIn(RGB_COLOROUT, LOW);
          delay(100);
          digitalWrite(RGB_S2, HIGH);
          digitalWrite(RGB_S3, HIGH);
          colorRange.greenMax = pulseIn(RGB_COLOROUT, LOW);
          delay(100);
          digitalWrite(RGB_S2, LOW);
          digitalWrite(RGB_S3, HIGH);
          colorRange.blueMax = pulseIn(RGB_COLOROUT, LOW);
          delay(100);
          Serial.println("SENSORS calibration COMPLETE!");
          digitalWrite(13, LOW);

          // Save color range to EEPROM memory
          EEPROM.put(0, colorRange);
        }
      #endif

      /*
        Finalising the Colour and returns the colour as a char 
        Colour Codes 
          r - Red 
          g - Green
          b - Blue
          y - Yellow
          n - None 
      */
      char detectedColor() {
        // Sensing Red Color
        digitalWrite(RGB_S2, LOW);
        digitalWrite(RGB_S3, LOW);
        redFrequency = pulseIn(RGB_COLOROUT, LOW);
        redColor = map(redFrequency, colorRange.redMin, colorRange.redMax, 255, 0);
        delay(100);

        // Sensing Green Color
        digitalWrite(RGB_S2,HIGH);
        digitalWrite(RGB_S3,HIGH);
        greenFrequency = pulseIn(RGB_COLOROUT, LOW);
        greenColor = map(greenFrequency, colorRange.greenMin, colorRange.greenMax, 255, 0);
        delay(100);

        // Sensing Blue Color
        digitalWrite(RGB_S2,LOW);
        digitalWrite(RGB_S3,HIGH);
        blueFrequency = pulseIn(RGB_COLOROUT, LOW);
        blueColor = map(blueFrequency, colorRange.blueMin, colorRange.blueMax, 255, 0);
        delay(100);

        // Limit the range for each color
        redColor = constrain(redColor, 0, 255);
        greenColor = constrain(greenColor, 0, 255);
        blueColor = constrain(blueColor, 0, 255);

        // Identifying the brightest color
        int maxVal = max(redColor, blueColor);
        maxVal = max(maxVal, greenColor);
        
        // Map new color values
        redColor = constrain(map(redColor, 0, maxVal, 0, 255), 0, 255);
        greenColor = constrain(map(greenColor, 0, maxVal, 0, 255), 0, 255);
        blueColor = constrain(map(blueColor, 0, maxVal, 0, 255), 0, 255);

        // Determine which color is most likely detected
        if (redColor > 250 && greenColor < 200 && blueColor < 200) {
          color = 'r'; // Red
        } else if (redColor < 200 && greenColor > 250 && blueColor < 200) {
          color = 'g'; // Green
        } else if (redColor < 200 && blueColor > 250) {
          color = 'b'; // Blue
        } else if (redColor > 200 && greenColor > 200 && blueColor < 100) {
          color = 'y'; // Yellow
        } else {
          color = 'n'; // No color
        }

        return color;
      }
  };
#endif

Comms comms;
Motors motors;
Sensors sensors;

void setup() {
  Serial1.begin(BAUD_RATE);

  #if DEBUG
    Serial.begin(BAUD_RATE);
    Serial.print("Entered DEBUG mode\n");
    comms.calibrate();
    sensors.calibrate();
  #endif
}

void loop() {

  char sensorsCommand = sensors.detectedColor();
  if (sensorsCommand != 'n') {
    state.enqueueCommand(sensorsCommand);
  }

  char commsCommand = comms.receivedCommand();
  if (commsCommand != 'n') {
    state.enqueueCommand(commsCommand);
  }

  if (state.commandQueueCount != 0) {
    switch (state.dequeueCommand()) {
      // Sensors & Comms: Stop train (such as at station)
      case 'r':
      case 's':
        break;
      
      // Sensors: Speed up or slow down
      case 'g':
        break;
      
      // Sensors & Comms: Emergency stop
      case 'b':
      case 'x':
        break;

      // Sensors & Comms: Change train direction
      case 'y':
      case 'c':
        break;

      // Comms: Start train
      case 'm':
        break;

      // Comms: Open/Close doors
      case 'd':
        break;
    }
  }

}
