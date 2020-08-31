/*
  State Machine, Motors & Sensors - Arduino MEGA (T1/Carriage 2)
  Team: T1_C3
  Members: Afroja Rowson Akter, Nicholas Drummond, Safal Maharjan, Jayakrithi Shivakumar & Nathan Lecompte
*/

#include <Servo.h>

#define DEBUG 0
#define ENABLE_SENSORS 0
#define ENABLE_MOTORS 0
#define ENABLE_COMMS 0
#define ENABLE_DOORS 0

/*
  Error codes:
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
char status_code = 0;

float train_speed = 0.0;
#if ENABLE_DOORS
    int train_doors_opened = 0;
#endif
char train_direction = 0;

void setup() {
    #if ENABLE_COMMS
        setup_comms();
    #endif
    #if ENABLE_SENSORS
        setup_sensors();
    #endif
    #if ENABLE_MOTORS
        setup_motors();
    #endif
}

void loop() {

}

#pragma region Comms
#if ENABLE_COMMS
void setup_comms() {

}

/*
  Send test command over comms and check that response is correct
*/
void calibrate_comms() {

}

/*
  Data is sent in this format: 
    < status code (0-9) >:< data type (a, d, s) >_< data >
*/
void send_comms_command() {

}

/*
  Data is received in this format:
    x = Emergency stop
    g = Start train
    s = Stop train
    c = Change train direction
    d = Open/close doors
*/
void received_comms_command() {

}
#endif
#pragma endregion


#pragma region Motors
#if ENABLE_MOTORS
void setup_motors() {
    // Attach motors
}

/*
  Test changing train direction and accelerate/decelerate
*/
void calibrate_motors() {

}

void set_train_acceleration() {

}

void set_train_direction() {
    
}
#endif
#pragma endregion


#pragma region Sensors
#if ENABLE_SENSORS
void setup_sensors() {

}

/*
  Check color sensors sense correct colors, and other stuff
*/
void calibrate_sensors() {

}

void received_sensor_command() {

}
#endif
#pragma endregion
