/*
  State Machine, Motors & Sensors - Arduino MEGA (Train 2)
  Team: T1_C3
  Members: Afroja Rowson Akter, Nicholas Drummond, Safal Maharjan, Jayakrithi Shivakumar & Nathan Lecompte
*/

#define DEBUG 1
#define ENABLE_SENSORS 1
#define ENABLE_MOTORS 0
#define ENABLE_COMMS 1
#define ENABLE_DOORS 1

#define MAX_COMMANDS 100
#define BAUD_RATE 9600

#define BT_RX_PIN 16
#define BT_TX_PIN 17

#define MOTOR_DOOR_PIN 8
#define MOTOR_DRIVER_PIN 9
#define MOTOR_DRIVER_MIN 600
#define MOTOR_DRIVER_MAX 2400
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

    #if ENABLE_DOORS
      int doorsOpen = 0;
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
  #include <AltSoftSerial.h>

  class Comms {
    public:
      AltSoftSerial BTSerial;

      /*
        Send data over Comms link; data is sent in this format: 
          < status code (0-9) : data type (a = acceleration, d = direction) _ data >
      */
      void sendData(char type, char data) {
        char message[10];
        snprintf(message, sizeof(message), "%s:%s_%s", ::state.trainStatus, type, data);
        #if DEBUG
          Serial.write("<");
          delay(100);
          Serial.write(message);
          delay(100);
          Serial.write(">");
        #else
          BTSerial.write("<");
          delay(100);
          BTSerial.write(message);
          delay(100);
          BTSerial.write(">");
        #endif
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
        #if DEBUG
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
        #else
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
        #endif
        return command;
      }
  };
  Comms comms;
#endif

#if ENABLE_MOTORS
  #include <Servo.h>
  #include <Encoder.h>
  #include <PID_v1.h>

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
        Encoder built-into DC motor to calculate velocity
      */
      Encoder trainEncoder = Encoder(ENCODER_PIN_A, ENCODER_PIN_B);
      volatile unsigned long trainEncoderCount, trainEncoderCountOld, trainEncoderCountNew = 0;
 
      /*
        Create PID instance to maintain speed (kinda like cruise-control)
      */
      double kp = 0, ki = 10, kd = 0, input = 0, output = 0, setpoint = 0;
      PID trainPID = PID(&input, &output, &setpoint, kp, ki, kd, DIRECT);

      #if ENABLE_DOORS
        /*
          Servo which controls train doors movement
        */
        Servo doorServo;
      #endif

      /*
        Set train into provided speed setting:
          0 = No movement (stopped)
          1 = Slow speed
          2 = Normal speed
      */
      void setTrainSpeedState(int state) {
        int startMicroseconds = 1500;
        
        if (::state.trainSpeedState == 1) {
          startMicroseconds = ::state.trainDirection
            ? 1500 + ((MOTOR_DRIVER_MAX - 1500) / 2) 
            : MOTOR_DRIVER_MIN + ((1500 - MOTOR_DRIVER_MIN) / 2);

        } else if (::state.trainSpeedState == 2) {
          startMicroseconds = ::state.trainDirection 
            ? MOTOR_DRIVER_MAX 
            : MOTOR_DRIVER_MIN;
        }

        switch (state) {
          case 0:
            trainMotor.writeMicroseconds(1500);
            #if DEBUG
              Serial.println(" - SET TRAIN SPEED: Stop");
            #endif
            break;

          case 1:
            trainMotor.writeMicroseconds(::state.trainDirection
              ? 1500 + ((MOTOR_DRIVER_MAX - 1500) / 2) 
              : MOTOR_DRIVER_MIN + ((1500 - MOTOR_DRIVER_MIN) / 2));
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

      void trainEncoderCountEvent() {
        trainEncoderCount += digitalRead(ENCODER_PIN_B) == HIGH ? 1 : -1;
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

      #if DEBUG
        void calibrate() {
          Serial.print("- Begin calibration for RGB sensor...");

          // Aiming at WHITE color
          Serial.println("- RGB sensor calibrating - Min range...");
          Serial.println("- Begin calibrating color: WHITE");

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
          Serial.println("- Begin calibrating color: BLACK");
          digitalWrite(13, LOW);
          delay(2000);

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

  #if ENABLE_COMMS && !DEBUG
    // Init Bluetooth Comms link
    comms.BTSerial.begin(BAUD_RATE);
  #endif

  #if ENABLE_SENSORS
    // Init RGB Sensors
    pinMode(RGB_S0, OUTPUT);
    pinMode(RGB_S1, OUTPUT);
    pinMode(RGB_S2, OUTPUT);
    pinMode(RGB_S3, OUTPUT);
    digitalWrite(RGB_S0, HIGH);
    digitalWrite(RGB_S1, LOW);
    pinMode(RGB_COLOROUT, INPUT);
  #endif

  #if ENABLE_MOTORS

    #if ENABLE_DOORS
      // Init Servo for doors
      pinMode(MOTOR_DOOR_PIN, OUTPUT);
      motors.doorServo.attach(MOTOR_DOOR_PIN);
    #endif

    // Init DC Motor for drive
    pinMode(MOTOR_DRIVER_PIN, OUTPUT);
    motors.trainMotor.attach(MOTOR_DRIVER_PIN, MOTOR_DRIVER_MIN, MOTOR_DRIVER_MAX);

    // Init PID
    motors.trainPID.SetMode(AUTOMATIC);
    motors.trainPID.SetTunings(motors.kp, motors.ki, motors.kd);

    // Init Encoder
    attachInterrupt(0, trainEncoderCountEvent, CHANGE);
  #endif

  #if DEBUG
    Serial.println(" - Entered DEBUG mode");
    #if ENABLE_SENSORS
      sensors.calibrate();
    #endif
  #endif
}

#if ENABLE_MOTORS
  /*
    Train encoder event for interrupt call
  */
  void trainEncoderCountEvent() {
    motors.trainEncoderCountEvent();
  }
#endif

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
          } else if (state.trainSpeedState == 1) {
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
        #endif
        #if DEBUG
          Serial.println(" - COMMAND: Start/Move train");
        #endif
        break;

      // Comms: Open/Close doors
      case 'd':
        #if ENABLE_MOTORS && ENABLE_DOORS
          motors.toggleTrainDoors();
        #endif
        #if DEBUG
          Serial.println(" - COMMAND: Open/Close doors");
        #endif
        break;
    }
  }

  #if ENABLE_MOTORS
    motors.input = motors.trainEncoderCount;
    motors.trainPID.Compute();
    // delay(1000);
    // comms.sendData('s', state.trainSpeed);
  #endif
}
