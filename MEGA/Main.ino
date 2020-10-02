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

#define MAX_COMMANDS 100
#define BAUD_RATE 9600

#define BT_NAME "enggmetro"
#define BT_PIN 8080
#define BT_RX_PIN 16
#define BT_TX_PIN 17

#define MOTOR_MAX_ACCELERATION 1700
#define MOTOR_MIN_ACCELERATION 1300
#define MOTOR_DOOR_PIN 8
#define MOTOR_TRAIN_PIN 9
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
  #include <SoftwareSerial.h>

  class Comms {
    private:
      SoftwareSerial BTSerial = SoftwareSerial(BT_RX_PIN, BT_TX_PIN);

    public:
      Comms() {
        BTSerial.begin(BAUD_RATE);
        delay(1000);
        BTSerial.write("AT+NAME" + BT_NAME);
        BTSerial.write("AT+PIN" + BT_PIN);
      }

      #if DEBUG
        void calibrate() {
          Serial.println("- Begin calibration for COMMS...");
          char response;
          BTSerial.write("C:1");
          delay(1000);
          if (BTSerial.available() != 0) {
            response = BTSerial.read();
            if (response == 'b') {
              Serial.print("- COMMS calibration SUCCESS!");
            } else {
              Serial.print("- COMMS calibration FAILED!");
            }
          }
        }
      #endif

      /*
        Data is sent in this format: 
          < status code (0-9) : data type (a, d, s) _ data >
      */
      bool sendCommand(char type, int data) {
        char message[10];
        if (type && data) {
          snprintf(message, sizeof message, "%s:%s_%s", state.status, type, data);
          BTSerial.write("<");
          delay(100);
          BTSerial.write(message);
          delay(100);
          BTSerial.write(">");
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
        while (BTSerial.available()) {
          response = BTSerial.read();
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
  Comms comms;
#endif

#if ENABLE_MOTORS
  #include <Servo.h>
  #include <Encoder.h>

  /* 
    Motor Driver: NeveRest Classic 60 Gearmotor
  */
  class Motors {
    private:
      Encoder trainEncoder = Encoder(ENCODER_PIN_A, ENCODER_PIN_B);
      int encoderCount = 0;
      int encoderLastCount;
      Servo trainMotor;
      Servo doorServo;

    public:
      Motors() {
        pinMode(MOTOR_TRAIN_PIN, OUTPUT);
        pinMode(MOTOR_DOOR_PIN, OUTPUT);
        trainMotor.attach(MOTOR_TRAIN_PIN);
        doorMotor.attach(MOTOR_DOOR_PIN);
        attachInterrupt(0, setTrainEncoder, CHANGE);
	      attachInterrupt(1, setTrainEncoder, CHANGE);
      }

      void easeTrainSpeed(unsigned long speed) {
        if (::state.speed != 1500) {
          if (speed > 1500 && speed < MOTOR_MAX_ACCELERATION) {
            trainMotor.writeMicroseconds(speed);
            while (::state.speed != 1500)
          } else if (speed < 1500 && speed < MOTOR_MIN_ACCELERATION) {

          }
        }
      }

      void setTrainSpeed(unsigned long speed) {
        trainMotor.writeMicroseconds(speed);
        ::state.speed = speed;
      }

      void setTrainEncoder() {
        encoderCount++;
      }

      #if ENABLE_DOORS
        void openTrainDoors() {
          if (::state.doorsOpen == 0) {
            for (int i = 0; i <= 95; i += 10) {
              doorServo.write(i);
              delay(250);
            }
            ::state.doorsOpen = 1;
          }
        }

        void closeTrainDoors() {
          if (::state.doorsOpen == 1) {
            for (int i = 190; i >= 95; i -= 10) {
              doorServo.write(i);
              delay(250);
            }
            ::state.doorsOpen = 0;
          }
        }
      #endif
  };
  Motors motors;
#endif

#if ENABLE_SENSORS
  #include <EEPROM.h>

  /* 
    RGB Sensor: XC3708
    Accelerometer Sensor: MPU6050
  */
  class Sensors {
    private:
      // Amount of primary color detection from the sensed object
      typedef struct Color {
        int red;
        int green;
        int blue;
      } Color;
      Color color;

      // Reads the Frequency of the respective colors 
      typedef struct Frequency {
        int red = 0;
        int green = 0;
        int blue = 0;
      } Frequency;
      Frequency colorFrequency;

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
          // Calibrate RGB sensor
          Serial.print("- Begin calibration for RGB sensor...");

          // Aiming at WHITE color
          Serial.println("- RGB sensor calibrating - Min range...");
          Serial.println("- Detected color: WHITE");

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
          Serial.println("- RGB sensor calibrating - Max range...");
          digitalWrite(13, LOW);
          delay(2000);
          Serial.println("- Detected color: BLACK");

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
          Serial.println("- RGB sensor calibration COMPLETE!");
          digitalWrite(13, LOW);

          // Save color range to EEPROM memory
          EEPROM.put(0, colorRange);

          // Calibrate Accelerometer sensor
          Serial.print("- Begin calibration for Accelerometer sensor...");

        }
      #endif

      /*
        Finalising the Color and returns the color as a char 
        Color Codes 
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
        colorFrequency.red = pulseIn(RGB_COLOROUT, LOW);
        color.red = map(colorFrequency.red, colorRange.redMin, colorRange.redMax, 255, 0);
        delay(100);

        // Sensing Green Color
        digitalWrite(RGB_S2,HIGH);
        digitalWrite(RGB_S3,HIGH);
        colorFrequency.green = pulseIn(RGB_COLOROUT, LOW);
        color.green = map(colorFrequency.green, colorRange.greenMin, colorRange.greenMax, 255, 0);
        delay(100);

        // Sensing Blue Color
        digitalWrite(RGB_S2,LOW);
        digitalWrite(RGB_S3,HIGH);
        colorFrequency.blue = pulseIn(RGB_COLOROUT, LOW);
        color.blue = map(colorFrequency.blue, colorRange.blueMin, colorRange.blueMax, 255, 0);
        delay(100);

        // Limit the range for each color
        color.red = constrain(color.red, 0, 255);
        color.green = constrain(color.green, 0, 255);
        color.blue = constrain(color.blue, 0, 255);

        // Identifying the brightest color
        int maxVal = max(color.red, color.blue);
        maxVal = max(maxVal, color.green);
        
        // Map new color values
        color.red = constrain(map(color.red, 0, maxVal, 0, 255), 0, 255);
        color.green = constrain(map(color.green, 0, maxVal, 0, 255), 0, 255);
        color.blue = constrain(map(color.blue, 0, maxVal, 0, 255), 0, 255);

        // Determine which color is most likely detected
        if (color.red > 250 && color.green < 200 && color.blue < 200) {
          return 'r'; // Red
        } else if (color.red < 200 && color.green > 250 && color.blue < 200) {
          return 'g'; // Green
        } else if (color.red < 200 && color.blue > 250) {
          return 'b'; // Blue
        } else if (color.red > 200 && color.green > 200 && color.blue < 100) {
          return 'y'; // Yellow
        }
        return 'n'; // No color
      }

      /*
        Estimated velocity (or speed) detected via the Accelerometer
      */
      int detectedSpeed() {

      }
  };
  Sensors sensors;
#endif

void setup() {
  Serial.begin(BAUD_RATE);
  Serial.println("Welcome to ENGG-METRO!");

  #if DEBUG
    Serial.println(" - Entered DEBUG mode");
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
        motors.setTrainAcceleration(1500);
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
