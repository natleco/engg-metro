//  SerialIn_SerialOut_HM-10_01
//
//  Uses hardware serial to talk to the host computer and AltSoftSerial for communication with the bluetooth module
//
//  What ever is entered in the serial monitor is sent to the connected device
//  Anything received from the connected device is copied to the serial monitor
//  Does not send line endings to the HM-10
//
//  Pins
//  BT VCC to Arduino 5V out.
//  BT GND to GND
//  Arduino D8 (SS RX) - BT TX no need voltage divider
//  Arduino D9 (SS TX) - BT RX through a voltage divider (5v to 3.3v)
//
#include <AltSoftSerial.h>
#include "Parse.h"
#include "TrainStatus.h"
AltSoftSerial BTserial;
// https://www.pjrc.com/teensy/td_libs_AltSoftSerial.html


boolean NL = true;
Parse parser = Parse();
TrainStatus trainStatus;

void setup() {
  while (!Serial);
  Serial.begin(9600);
  BTserial.begin(9600);
  Serial.println("BTserial started at 9600");
}

void loop() {
  considerReadingInput();
  if (Serial.available())
  {
    char c = ' ';
    c = Serial.read();

    // do not send line end characters to the HM-10
    if (c != 10 & c != 13 )
    {
      BTserial.write(c);
    }

    // Echo the user input to the main window.
    // If there is a new line print the ">" character.
    if (NL) {
      NL = false;
    }
    Serial.write(c);
    if (c == 10) {
      NL = true;
    }
  }
}

void considerReadingInput() {
  String input = "";
  while (BTserial.available()) {
    input = input + (char) BTserial.read();
    delay(100);
  }

  if (input.length() > 0) {
    trainStatus = parser.parse(input);
  }
}
