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
  };
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
        // Use ::state to access the state class in this scope
	      pinMode(RGB_S0, OUTPUT);
        pinMode(RGB_S1, OUTPUT);
        pinMode(RGB_S2,OUTPUT);
        pinMode(RGB_S3,OUTPUT);
        pinMode(13,OUTPUT); // Tested for UNO
        pinMode(RGB_COLOUROUT,INPUT);
      }

    // Calibrating function
    void calibrate(){
      // Aiming at WHITE colour
      Serial.println("Sensors Calibrating - Min range...");
      Serial.println(" Sensed colour : White");

      //setting calibration values - Min range:
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

      //Aiming at BLACK colour
      Serial.println("next...");
      Serial.println("Sensors Calibrating - Max range...");
      digitalWrite(13, LOW);
      delay(2000);
      Serial.println("Black");

      // setting calibration values- Max range:
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
	    // Sensing Red Colour 
	    digitalWrite(RGB_S2,LOW);
	    digitalWrite(RGB_S3,LOW);
      redFrequency = pulseIn(RGB_COLOUROUT,LOW);
      redColour = map(redFrequency , redMin, redMax,255,0);
      delay(100);

	    // Sensing Green Colour 
	    digitalWrite(RGB_S2,HIGH);
    	digitalWrite(RGB_S3,HIGH);
      greenFrequency = pulseIn(RGB_COLOUROUT,LOW);
      greenColour = map(greenFrequency , greenMin, greenMax,255,0);
      delay(100);

	    // Sensing Blue Colour 
	    digitalWrite(RGB_S2,LOW);
	    digitalWrite(RGB_S3,HIGH);
      blueFrequency = pulseIn(RGB_COLOUROUT,LOW);
      blueColour = map(blueFrequency , blueMin, blueMax,255,0);
      delay(100);     
	  }

    /* Finalising the Colour and returns the colour as a char 
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
      else if (redColour < 200 /*&& greenColor < 200*/ && blueColour > 250) {
        colour = 'b'; //Blue
      }
      else if (redColour > 200 &&  greenColour > 200 && blueColour < 100) {
        colour = 'y'; //Yellow
      }
      else colour = 'N'; // None
      return colour;
    }
    
    // Printing the colour values in the serial monitor
    void printColour() {
      Serial.print("R = ");
      Serial.print(redColour);
      Serial.print(" G = ");
      Serial.print(greenColour);
      Serial.print(" B = ");
      Serial.print(blueColour);
      
      Serial.print(" Color: ");
      switch (colour) {
        case 'r': Serial.println("RED"); break;
        case 'g': Serial.println("GREEN"); break;
        case 'b': Serial.println("BLUE"); break;
        case 'y': Serial.println("YELLOW"); break;
        default: Serial.println("unknown"); break;
      }
    }
  
   // Invokes the functions needed to read and detect the colour from the sensed object
    char enableSensors(){
      readColours(); // printColour(); To Verify
     return detectColour();
    }
  };
   
#endif
Comms comms();
Motors motors();
Sensors sensors();

void setup() {
  #if DEBUG
    Serial1.begin(9600);
    Serial1.print("Entered DEBUG mode\n");
  #endif
  //Sensors calibration mode
  sensors.calibrate();
}

void loop() {
sensors.enableSensors(); // Enabling Sensors to detect colour
}
