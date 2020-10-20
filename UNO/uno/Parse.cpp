#include "Arduino.h"
#include "Parse.h"

char dirDelim = 'd';
char speedDelim = 'a';
TrainStatus train;

Parse::Parse() { }

TrainStatus Parse::parse(String input) {
  input.trim();
  if (input.charAt(0) == startDelim && input.charAt(input.length() - 1) == termDelim) {
    if (hasData(input, dirDelim)) {
      train.status = input.substring(input.indexOf(startDelim) + 1, input.indexOf(statDelim));
      train.dir = parseData(input, dirDelim);
    }

    if (hasData(input, speedDelim)) {
      train.status = input.substring(input.indexOf(startDelim) + 1, input.indexOf(statDelim));
      train.speed = parseData(input, speedDelim);

    }
  }
  return train;
}

String Parse::parseData(String input, char dataDelim) {
  return input.substring(input.indexOf(dataDelim) + 2, input.indexOf(Parse::termDelim));
}

bool Parse::hasData(String input, char dataDelim) {
  return input.indexOf(dataDelim) != -1;
}
