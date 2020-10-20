#ifndef Parse_h
#define Parse_h
#include "TrainStatus.h"

class Parse {
  
  private:
    char startDelim = '<';
    char termDelim = '>';
    char statDelim = ':';
    String parseData(String input, char dataDelim);

  public:
    Parse();
    TrainStatus parse(String input);
    bool hasData(String input, char dataDelim);
};
#endif
