#include <Arduino.h>

#define NEC_BIT_PER_BLOCK 32

class NECIRReceiver {

public:
NECIRReceiver(int irPinArg);
int getIRButtonValue();
int getIrPin();
void setIrPin(int irPinArg);

private:
NECIRReceiver();
int getIRButtonValue(int irPinArg);

int irPin;
static int start_bit;
static int repeat_bit;
static int bin_1;
static int bin_0;

};




