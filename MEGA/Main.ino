/*
  State Machine, Motors & Sensors - Arduino MEGA (Train 2)
  Team: T1_C3
  Members: Afroja Rowson Akter, Nicholas Drummond, Safal Maharjan, Jayakrithi Shivakumar & Nathan Lecompte
*/

#define DEBUG 1
#define ENABLE_SENSORS 1
#define ENABLE_MOTORS 1
#define ENABLE_COMMS 1

#define MAX_COMMANDS 100
#define BAUD_RATE 9600

#define BT_RX_PIN 17
#define BT_TX_PIN 16

#define MOTOR_DOOR_PIN 8
#define MOTOR_DRIVER_PIN 9
#define MOTOR_DRIVER_MIN 1200
#define MOTOR_DRIVER_MAX 1800
#define ENCODER_PIN_A 2
#define ENCODER_PIN_B 3
#define EMERGENCY_PIN 13

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
      State of train speed; values include:
        0 = No movement (stopped)
        1 = Slow speed
        2 = Normal speed
    */
    int trainSpeedState = 0;

    /*
      Actual speed of train in RPM
    */
    float trainSpeed = 0;

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
  /* 
    Bluetooth Module: HM-10
  */
  class Comms {
    public:

      /*
        Send data over Comms link; data is sent in this format: 
          < status code (0-9) : data type (a = acceleration, d = direction) _ data >
      */
      void sendData(char type, char data) {
        char message[10];
        snprintf(message, sizeof(message), "<%s:%s_%s>", ::state.trainStatus, type, data);
        Serial.write(message);
      }

      /*
        Handle received command over Comms link; commands recognised include:
          x = Emergency stop
          m = Start train
          s = Stop train
          c = Change train direction
      */
      char receivedCommand() {
        char response;
        char command = 'n';
        while (Serial.available()) {
          response = Serial.read();
          if (response != '<' && response != '>') {
            command = response;
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

  /* 
    Motor Driver: REV Robotics SPARK Motor Controller
    Motor: NeveRest Classic 60 Gearmotor
  */
  class Motors {
    public:
      
      /*
        DC motor which controls train movement
      */
      Servo trainMotor;

      /*
        Set train into provided speed setting:
          0 = No movement (stopped)
          1 = Slow speed
          2 = Normal speed
      */
      void setTrainSpeedState(int state) {
        switch (state) {
          case 0:
            trainMotor.writeMicroseconds(1500);
            #if DEBUG
              Serial.println(" - SET TRAIN SPEED: Stop");
            #endif
            break;

          case 1:
            trainMotor.writeMicroseconds(::state.trainDirection
              ? (1500 + ((MOTOR_DRIVER_MAX - 1500) / 2)) 
              : (MOTOR_DRIVER_MIN + ((1500 - MOTOR_DRIVER_MIN) / 2)));
            #if DEBUG
              Serial.println(" - SET TRAIN SPEED: Slow");
            #endif
            break;

          case 2:
            trainMotor.writeMicroseconds(::state.trainDirection 
              ? MOTOR_DRIVER_MAX 
              : MOTOR_DRIVER_MIN);
            #if DEBUG
              Serial.println(" - SET TRAIN SPEED: Normal");
            #endif
            break;
        }
        ::state.trainSpeedState = state;
      }

      /*
        Trigger relay to emergency stop
      */
      void emergencyStop() {
        digitalWrite(EMERGENCY_PIN, HIGH);
        
      }
  };
  Motors motors;
#endif

#if ENABLE_SENSORS
  /* 
    RGB Sensor: XC3708
    Accelerometer Sensor: MPU6050
  */
  class Sensors {
    public:

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
        Determine which color is detected by RGB/Color sensor; returns:
          r = Red 
          g = Green
          b = Blue
          y = Yellow
          n = None
      */
      char detectedColor() {
        // Detect RED
        digitalWrite(RGB_S2, LOW);
        digitalWrite(RGB_S3, LOW);
        color.red = pulseIn(RGB_COLOROUT, LOW);

        // Detect GREEN
        digitalWrite(RGB_S2, HIGH);
        digitalWrite(RGB_S3, HIGH);
        color.green = pulseIn(RGB_COLOROUT, LOW);

        // Detect BLUE
        digitalWrite(RGB_S2, LOW);
        digitalWrite(RGB_S3, HIGH);
        color.blue = pulseIn(RGB_COLOROUT, LOW);

        #if DEBUG
          char colorValues[128];
         snprintf(colorValues, sizeof(colorValues), "--- Detected colors: rgb(%i, %i, %i)", color.red, color.green, color.blue);
         Serial.println(colorValues);
        #endif

        // Determine which color is most likely detected
        if (color.red > 10 && color.red < 22 && color.green > 45 && color.green < 52 && color.blue > 36 && color.blue < 42) {
          return 'r'; // Red
        } else if (color.red > 22 && color.red < 30 && color.green > 19 && color.green < 27 && color.blue > 30 && color.blue < 40) {
          return 'g'; // Green
        } else if (color.red > 36 && color.red < 43 && color.green > 25 && color.green < 32 && color.blue > 10 && color.blue < 22) {
          return 'b'; // Blue
        } else if (color.red > 6 && color.red < 20 && color.green > 6 && color.green < 25 && color.blue > 15 && color.blue < 38) {
          return 'y'; // Yellow
        }

        return 'n'; // None
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
    // Init Bluetooth Comms link
    Serial.begin(BAUD_RATE);
  #endif

  #if ENABLE_SENSORS
    // Init RGB Sensors
    pinMode(RGB_S0, OUTPUT);
    pinMode(RGB_S1, OUTPUT);
    pinMode(RGB_S2, OUTPUT);
    pinMode(RGB_S3, OUTPUT);
    digitalWrite(RGB_S0, HIGH);
    digitalWrite(RGB_S1, HIGH);
    pinMode(RGB_COLOROUT, INPUT);
  #endif

  #if ENABLE_MOTORS
    // Init relay for emergency break
    pinMode(EMERGENCY_PIN, OUTPUT);
    // Init DC Motor for drive
    pinMode(MOTOR_DRIVER_PIN, OUTPUT);
    motors.trainMotor.attach(MOTOR_DRIVER_PIN, MOTOR_DRIVER_MIN, MOTOR_DRIVER_MAX);
  #endif

  #if DEBUG
    Serial.println(" - Entered DEBUG mode");
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
    char command = state.dequeueCommand();
    switch (command) {
      // Sensors & Comms: Stop train (such as at station)
      case 'r':
      case 's':
        #if ENABLE_MOTORS
          motors.setTrainSpeedState(0);
        #endif
        #if DEBUG
          Serial.println(" - COMMAND: Stop train at station");
        #endif
        break;
      
      // Sensors: Speed up or slow down
      case 'g':
        #if ENABLE_MOTORS
          if (state.trainSpeedState == 2) {
            motors.setTrainSpeedState(1);
          } else if(state.trainSpeedState == 1) {
            motors.setTrainSpeedState(2);
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
          motors.setTrainSpeedState(0);
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
          motors.setTrainSpeedState(state.trainSpeedState);
          comms.sendData('d', state.trainDirection);
        #endif
        #if DEBUG
          Serial.println(" - COMMAND: Change train direction");
        #endif
        break;

      // Comms: Start/Move train
      case 'm':
        #if ENABLE_MOTORS
          motors.setTrainSpeedState(1);
          // Disable emergency stop break
          digitalWrite(EMERGENCY_PIN, LOW);
        #endif
        #if DEBUG
          Serial.println(" - COMMAND: Start/Move train");
        #endif
        break;
    }
  }
}
