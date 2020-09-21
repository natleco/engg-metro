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
#define RGB_COLOUROUT 8
#define RGB_S2 7
#define RGB_S3 6
#define RGB_S1 5 
#define RGB_S0 4


  class Sensors {
    /* Sensor Type : RGB sensors 
       Sensor Model : XC3708 
       Number of sensors : 1 */

    public:
      // Reads the Frequency of the respective colours 
      int redFrequency =0;
      int greenFrequency =0;
      int blueFrequency =0;

      // The final colour
      char colour;

      /* Amount of primary colour detection from the sensed object*/
      int redColour;
      int greenColour;
      int blueColour;
      
      /* Minimum and Maximum values for calibration */
      int redMin;
      int redMax;
      int greenMin;
      int greenMax;
      int blueMin;
      int blueMax;
    
      Sensors() {
        /* Use ::state to access the state class in this scope */
        pinMode(RGB_S0, OUTPUT);
        pinMode(RGB_S1, OUTPUT);
        pinMode(RGB_S2,OUTPUT);
        pinMode(RGB_S3,OUTPUT);
        pinMode(13,OUTPUT); // Tested for UNO
        pinMode(RGB_COLOUROUT,INPUT);
      }

    // Calibrating function
    void calibrate(){
      /* Aiming at WHITE colour */
      Serial.println("Sensors Calibrating - Min range...");
      Serial.println(" Sensed colour : White");

      /*setting calibration values - Min range: */
      digitalWrite(13, HIGH);
      delay(2000);
      digitalWrite(RGB_S2, LOW);
      digitalWrite(RGB_S3, LOW);
      redMin = pulseIn(RGB_COLOUROUT, LOW);
      delay(100);

      digitalWrite(RGB_S2, HIGH);
      digitalWrite(RGB_S3, HIGH);
      greenMin = pulseIn(RGB_COLOUROUT, LOW);
      delay(100);

      digitalWrite(RGB_S2, LOW);
      digitalWrite(RGB_S3, HIGH);
      blueMin = pulseIn(RGB_COLOUROUT, LOW);
      delay(100);

      /*Aiming at BLACK colour */
      Serial.println("next...");
      Serial.println("Sensors Calibrating - Max range...");
      digitalWrite(13, LOW);
      delay(2000);
      Serial.println("Black");

      /* setting calibration values- Max range: */
      digitalWrite(13, LOW);
      delay(2000);
      digitalWrite(RGB_S2, LOW);
      digitalWrite(RGB_S3, LOW);
      redMax = pulseIn(RGB_COLOUROUT, LOW);
      delay(100);
      digitalWrite(RGB_S2, HIGH);
      digitalWrite(RGB_S3, HIGH);
      greenMax = pulseIn(RGB_COLOUROUT, LOW);
      delay(100);
      digitalWrite(RGB_S2, LOW);
      digitalWrite(RGB_S3, HIGH);
      blueMax = pulseIn(RGB_COLOUROUT, LOW);
      delay(100);
      Serial.println("Calibration Complete ");
      digitalWrite(13, LOW);
    }

    // Reads the colour intensity of every primary colour from the sensed object
    void readColours() {
      /* Sensing Red Colour  */
      digitalWrite(RGB_S2,LOW);
      digitalWrite(RGB_S3,LOW);
      redFrequency = pulseIn(RGB_COLOUROUT,LOW);
      redColour = map(redFrequency , redMin, redMax,255,0);
      delay(100);

      /* Sensing Green Colour */
      digitalWrite(RGB_S2,HIGH);
      digitalWrite(RGB_S3,HIGH);
      greenFrequency = pulseIn(RGB_COLOUROUT,LOW);
      greenColour = map(greenFrequency , greenMin, greenMax,255,0);
      delay(100);

      /* Sensing Blue Colour */
      digitalWrite(RGB_S2,LOW);
      digitalWrite(RGB_S3,HIGH);
      blueFrequency = pulseIn(RGB_COLOUROUT,LOW);
      blueColour = map(blueFrequency , blueMin, blueMax,255,0);
      delay(100);     
    }

    /*Finalising the Colour and returns the colour as a char 
      Colour Codes 
        r - Red 
        g - Green
        b - Blue
        y - Yellow
        N - None 
    */ 

    char detectColour(){
      //Limit the range for each colour:
      redColour = constrain(redColour, 0, 255);
      greenColour = constrain(greenColour, 0, 255);
      blueColour = constrain(blueColour, 0, 255);

      //Identifying the brightest color:
      int maxVal = max(redColour, blueColour);
      maxVal = max(maxVal, greenColour);
      
      //map new values
      redColour = map(redColour, 0, maxVal, 0, 255);
      greenColour = map(greenColour, 0, maxVal, 0, 255);
      blueColour = map(blueColour, 0, maxVal, 0, 255);
      redColour = constrain(redColour, 0, 255);
      greenColour = constrain(greenColour, 0, 255);
      blueColour = constrain(blueColour, 0, 255);

      /* Finalising which colour is present and assigning it to colour variable.
         RANGE VALUES MAY VARY  */
      if (redColour > 250 && greenColour < 200 && blueColour < 200) {
        colour = 'r'; //Red
      }
      else if (redColour < 200 && greenColour > 250 && blueColour < 200) {
        colour = 'g'; //Green
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
  sensors.calibrate();
}

void loop() {
  sensors.enableSensors(); 
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
