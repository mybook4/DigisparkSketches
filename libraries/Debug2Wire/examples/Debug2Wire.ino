#include "Debug2Wire.h"

Debug2Wire gpioDebugger(14, 15, 1);

void setup() {}

void loop() {
  gpioDebugger.debugPrintMSb(0x5A);
}
