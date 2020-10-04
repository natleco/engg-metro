/*
  State Machine, Motors & Sensors - Arduino MEGA (Train 2)
  Team: T1_C3
  Members: Afroja Rowson Akter, Nicholas Drummond, Safal Maharjan, Jayakrithi Shivakumar & Nathan Lecompte
*/

#define DEBUG 1
#define ENABLE_SENSORS 1
#define ENABLE_MOTORS 1
#define ENABLE_COMMS 1
#define ENABLE_DOORS 1

#define MAX_COMMANDS 100
#define BAUD_RATE 9600

#define BT_RX_PIN 16
#define BT_TX_PIN 17

#define MOTOR_DOOR_PIN 8
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
      Status codes; values include:
        0 = Stopped at station state
        1 = Moving state
        2 = Emergency stopped state
        3 = Unrecognised command error
    */
    int trainStatus = 0;

    /*
      Direction of train; values include:
        0 = Forwards
        1 = Backwards
    */
    int trainDirection = 0;

    /*
      Speed of train; values include:
        0 = No movement (stopped)
        1 = Slow speed
        2 = Normal speed
      
      TODO: Use more accurate speed values
    */
    int trainSpeed = 0;

    #if ENABLE_DOORS
      char doorsOpen = 0;
    #endif

    /*
      Queue of commands received from Sensors or Comms
    */
    char commandQueue[MAX_COMMANDS];
    int commandQueueBack = -1;
    int commandQueueFront = 0;
    int commandQueueCount = 0;

    /*
      Add command to commandQueue
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
      Remove oldest command from commandQueue
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
    public:
      SoftwareSerial BTSerial = SoftwareSerial(BT_RX_PIN, BT_TX_PIN);

      /*
        Send data over Comms link; data is sent in this format: 
          < status code (0-9) : data type (a = acceleration, d = direction, s = station) _ data >
      */
      void sendData(char type, char data) {
        char message[10];
        snprintf(message, sizeof(message), "%s:%s_%s", ::state.trainStatus, type, data);
        BTSerial.write("<");
        delay(100);
        BTSerial.write(message);
        delay(100);
        BTSerial.write(">");
      }

      /*
        Handle received command over Comms link; commands recognised include:
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
    Motor Driver: REV Robotics SPARK Motor Controller
    Motor: NeveRest Classic 60 Gearmotor
  */
  class Motors {
    private:
      Encoder trainEncoder = Encoder(ENCODER_PIN_A, ENCODER_PIN_B);
      int trainEncoderCount = 0;
      Servo doorServo;

    public:
      Motors() {
        pinMode(MOTOR_DOOR_PIN, OUTPUT);
        doorServo.attach(MOTOR_DOOR_PIN);
      }

      /*
        Ease train into provided speed setting:
          0 = No movement (stopped)
          1 = Slow speed
          2 = Normal speed
      */
      void easeTrainSpeed(int speed) {
        // TODO: Write ease train speed function
        ::state.trainSpeed = speed;
      }

      void setTrainSpeed(int speed) {
        // TODO: Write set train speed function
        ::state.trainSpeed = speed;
      }

      #if ENABLE_DOORS
        void toggleTrainDoors() {
          if (::state.doorsOpen == 0) {
            for (int i = 0; i <= 95; i += 10) {
              doorServo.write(i);
              delay(250);
            }
            ::state.doorsOpen = 1;
          } else if (::state.doorsOpen == 1) {
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
      /*
        Amount of color detected from sensed object by RGB/Color sensor
      */
      typedef struct Color {
        int red;
        int green;
        int blue;
      } Color;
      Color color;

      /*
        Frequency of colors detected from sensed object by RGB/Color sensor
      */
      typedef struct Frequency {
        int red = 0;
        int green = 0;
        int blue = 0;
      } Frequency;
      Frequency colorFrequency;

      /*
        Minimum & maximum of colors detected during calibration by RGB/Color sensor
      */
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

        // TODO: Change for MEGA; only Tested for UNO
        pinMode(13, OUTPUT); 

        pinMode(RGB_COLOROUT, INPUT);
      }

      #if DEBUG
        void calibrate() {
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
        Determine which color is detected by RGB/Color sensor; returns:
          r = Red 
          g = Green
          b = Blue
          y = Yellow
          n = None 
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
        Determine whether movement is detected via the Accelerometer; returns:
          0 = No movement
          1 = Movement
      */
      int detectedMovement() {
        // TODO: Write movement detection function
      }
  };
  Sensors sensors;
#endif

void setup() {
  Serial.begin(BAUD_RATE);
  Serial.println("Welcome to ENGG-METRO!");

  #if ENABLE_COMMS
    comms.BTSerial.begin(BAUD_RATE);
    delay(2000);
    Serial.println("AT+NAME=enggmetro");
    delay(2000);
    Serial.println("AT+PSWD=8080");
    delay(2000);
  #endif

  #if DEBUG
    Serial.println(" - Entered DEBUG mode");
    #if ENABLE_SENSORS
      sensors.calibrate();
    #endif
  #endif
}

void loop() {
  #if ENABLE_SENSORS
    char sensorsCommand = sensors.detectedColor();
    if (sensorsCommand != 'n') {
      state.enqueueCommand(sensorsCommand);
    }
  #endif

  #if ENABLE_COMMS
    char commsCommand = comms.receivedCommand();
    if (commsCommand != 'n') {
      state.enqueueCommand(commsCommand);
    }
  #endif

  if (state.commandQueueCount != 0) {
    switch (state.dequeueCommand()) {
      // Sensors & Comms: Stop train (such as at station)
      case 'r':
      case 's':
        #if ENABLE_MOTORS
          motors.easeTrainSpeed(0);
        #endif
        #if DEBUG
          Serial.println(" - COMMAND: Stop train at station");
        #endif
        break;
      
      // Sensors: Speed up or slow down
      case 'g':
        #if ENABLE_MOTORS
          if (state.trainSpeed == 2) {
            motors.easeTrainSpeed(1);
          } else if (state.trainSpeed == 1) {
            motors.easeTrainSpeed(2);
          }
        #endif
        #if DEBUG
          Serial.println(" - COMMAND: Slow down or speed up");
        #endif
        break;
      
      // Sensors & Comms: Emergency stop
      case 'b':
      case 'x':
        #if ENABLE_MOTORS
          motors.setTrainSpeed(0);
        #endif
        #if DEBUG
          Serial.println(" - COMMAND: Emergency stop");
        #endif
        break;

      // Sensors & Comms: Change train direction
      case 'y':
      case 'c':
        #if ENABLE_MOTORS
          state.trainDirection = !state.trainDirection;
          motors.easeTrainSpeed(state.trainSpeed);
        #endif
        #if DEBUG
          Serial.println(" - COMMAND: Change train direction");
        #endif
        break;

      // Comms: Start/Move train
      case 'm':
        #if ENABLE_MOTORS
          motors.easeTrainSpeed(1);
        #endif
        #if DEBUG
          Serial.println(" - COMMAND: Start/Move train");
        #endif
        break;

      // Comms: Open/Close doors
      case 'd':
        #if ENABLE_MOTORS
          motors.toggleTrainDoors();
        #endif
        #if DEBUG
          Serial.println(" - COMMAND: Open/Close doors");
        #endif
        break;

      default:
        state.trainStatus = 3;
        #if DEBUG
          Serial.println(" - ERROR : COMMAND not recognised");
        #endif
        break;
    }
  }
}
