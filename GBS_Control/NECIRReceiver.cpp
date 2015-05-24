#include "NECIRReceiver.h"

int NECIRReceiver::start_bit = 3000; //Start bit threshold (Microseconds)
int NECIRReceiver::repeat_bit = 1700; //Repeat bit threshold (Microseconds)
int NECIRReceiver::bin_1 = 900; //Binary 1 threshold (Microseconds)
int NECIRReceiver::bin_0 = 400; //Binary 0 threshold (Microseconds)


NECIRReceiver::NECIRReceiver() {}

NECIRReceiver::NECIRReceiver(int irPinArg) {
  irPin = irPinArg;
}

int NECIRReceiver::getIrPin() {
  return irPin;
}

void NECIRReceiver::setIrPin(int irPinArg) {
  irPin = irPinArg;
  pinMode(irPin, INPUT);
}


int NECIRReceiver::getIRButtonValue() {
  return getIRButtonValue(irPin);
}


// decode infrared signal
int NECIRReceiver::getIRButtonValue(int irPinArg) {
  int data[NEC_BIT_PER_BLOCK];
  int i;
  int test; 
 
  test = pulseIn(irPinArg, HIGH); 
  if(test > start_bit) //tests for the 4.5 ms start bit
  {
    for(i = 0 ; i < NEC_BIT_PER_BLOCK ; i++) 
      data[i] = pulseIn(irPinArg, HIGH); //Start measuring bits, I only want HIGH pulses
 
    delay(100);  
    for(i = 0 ; i < NEC_BIT_PER_BLOCK ; i++) //Parse them
    {   
      if(data[i] > bin_1) //is it a 1?
        data[i] = 1;
      else if(data[i] > bin_0) //is it a 0?
        data[i] = 0;
      else
        return -2; //Flag the data as invalid; Return -2 on invalid data
    }
    
    //based on NEC protocol, command data started from bit 16
    //and end with bit 24 (8 bits long)
    int result = 0;
    for(i = 16 ; i < 24; i++) { 
      if(data[i] == 1) result |= (1<<i-16); //Convert data bits to integer
    }  
    return result; //Return key number
 
  }
  else if(test > repeat_bit) return -1; //Tests for the 2.5 ms repeat bit and sends -1 if true
 
}
