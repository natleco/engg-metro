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
     
      char command;

    public:
      
      Comms() {
        Serial.begin(BAUD_RATE);

      }

      /*
        Send test command over comms and check that response is correct
      */
      bool calibrate() {

        //this is sending the request over comms
        Serial.print("C:1")
        //delaying, to wait for a response 
        delay(100);

        //checking is response is available now
        if(Serial.available() && Serial.availableForWrite()){
          //checking the response
          if(Serial.read == "b"){
            return true;
          }
        }
        return false;
        
        // Send a test request over comms and await a response over comms
      }

      /*
        Data is sent in this format: 
          < status code (0-9) >:< data type (a, d, s) >_< data >
      */
      bool sendCommand(char type, int data) {
        if (type && data) {
          char message[10] = (::state.status + ":" + type + "_" + data);
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

        while(Serial.available()){
          //assuming it is in the structure: "<" + command ">"
          //command: any of the ones specified in the beginning: x, g, s, c or d

          if(Serial.read()!="<" && Serial.read()!=">"){
            //this is the command received
            command = Serial.read();
          }
        //this is the end of the command
          if(Serial.read()==">"){
            break;
          }
        }

        return command;

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

  switch(receiveCommand()){
    
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
