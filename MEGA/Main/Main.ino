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

    // Speed of train
    float speed = 0.0;

    #if ENABLE_DOORS
      // Flag if doors are open
      char doorsOpen = 0;
    #endif
};

State state();

#if ENABLE_COMMS
  class Comms {
    private: 
      int baudRate = 9600;

    public:
      Comms() {
        Serial.begin(baudRate);
        Serial1.begin(baudRate);
      }

      /*
        Send test command over comms and check that response is correct
      */
      void calibrate() {
        // Send a test request over comms and await a response over comms
      }

      /*
        Data is sent in this format: 
          < status code (0-9) >:< data type (a, d, s) >_< data >
      */
      bool sendCommand(char type, int data) {
        if (type && data) {
          char message[10] = (::state.status + ":" + type + "_" + data);
          Serial.write("<");
          delay(100);
          Serial.write(message);
          delay(100);
          Serial.write(">");
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
      void receivedCommand() {

      }
  }
#endif

#if ENABLE_MOTORS
  #include <Servo.h>
  #include <Encoder.h>
  class Motors {
    public:
      Motors() {
        // Use ::state to access the state class in this scope
      }

      /*
        Test changing train direction and accelerate/decelerate
      */
      void calibrate() {
        
      }

      void setAcceleration(int acceleration) {
        switch (acceleration) {
          case 0: 
            //
        }
      }

      void changeDirection() {

      }
  }
#endif

#if ENABLE_SENSORS
#define RGB_1_COLOUROUT A5
#define RGB_1_S2 A3
#define RGB_1_S3 A4
#define RGB_2_COLOUROUT A10
#define RGB_2_S2 A8
#define RGB_2_S3 A9 

  class Sensors {
    /* Sensor Type : RGB sensors 
       Sensor Model : XC3708 
       Number of sensors : 2 (Front and Rear of the carriage) */

    private: 
      int baudRate = 9600;

    public:
    
      Sensors() {
        // Use ::state to access the state class in this scope
        Serial.begin(baudRate);
        pinMode(RGB_1_COLOUROUT,INPUT);
        pinMode(RGB_1_S2,OUTPUT);
        pinMode(RGB_1_S3,OUTPUT);

        /*  When RGB_2 sensors are enabled,
        ie: When carriage's direction is reversed 
        the sensors at the current Front section of
        the carriage will be enabled 

        pinMode(RGB_2_COLOUROUT,INPUT); 
        pinMode(RGB_2_S2,OUTPUT);
        pinMode(RGB_2_S3,OUTPUT); */

      }

      /*
        Check color sensors sense correct colors, and other stuff
      */
      void calibrate() {
        int intensity = getIntensity();

        // Detecting the percentage of Red colour 
        detectRed();
        Serial.print("RED :");
        Serial.print(getIntensity());
        Serial.print("% ");

        // Detecting the percentage of Green colour 
        detectGreen();
        Serial.print("GREEN :");
        Serial.print(getIntensity());
        Serial.print("% ");        
      }

      /*Checks for Red in the colour detected */
      void detectRed(){
        digitalWrite(RGB_1_S2,LOW);
        digitalWrite(RGB_1_S3,LOW);
      }

      /*Checks for Green in the colour detected */
      void detectGreen(){ 
        digitalWrite(RGB_1_S2,HIGH);
        digitalWrite(RGB_1_S3,HIGH);
      }

      //measure intensity with oversampling
      int getIntensity(){ 
      int a=0;
      int b=255;
      for(int i=0;i<10;i++)
      {
        /* pulseIn waits for the pin to go from HIGH to LOW
           and returns the number of microseconds to wait for the 
           pulse to start (Default = 1 second) */
        a = a + pulseIn(RGB_1_COLOUROUT,LOW);
      }
      if(a > 9)
      {
        b = 2550/a;
      }
      return b;
      }

      void receivedCommand() {

      }
  }
#endif

Comms comms();
Motors motors();
Sensors sensors();

void setup() {
  #if DEBUG
    Serial1.begin(9600);
    Serial1.print("Entered DEBUG mode\n");
  #endif
}

void loop() {

}
