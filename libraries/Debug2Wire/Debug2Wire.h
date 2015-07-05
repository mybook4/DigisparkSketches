#ifndef _DEBUG2WIRE_H_
#define _DEBUG2WIRE_H_

#include "Arduino.h"

class Debug2Wire {
public:
  Debug2Wire(int clkPinArg, int dataPinArg);
  void debugPrintMSb(uint8_t data);
  void debugPrintLSb(uint8_t data);

private:
  Debug2Wire();
  int clkPin;
  int dataPin;
};


#endif
