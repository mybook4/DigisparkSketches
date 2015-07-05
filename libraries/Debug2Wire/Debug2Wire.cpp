#include "Debug2Wire.h"

Debug2Wire::Debug2Wire(int clkPinArg, int dataPinArg) {
  clkPin = clkPinArg;
  dataPin = dataPinArg;

  pinMode(clkPin, OUTPUT);
  digitalWrite(clkPin, LOW);

  pinMode(dataPin, OUTPUT);
  digitalWrite(dataPin, LOW);
}

void Debug2Wire::debugPrintMSb(uint8_t data) {

  // print out the byte MSb to LSb

  uint8_t mask = 0x80;
  for(int i=0; i < 8; i++) {

    if(data & mask) {
      digitalWrite(dataPin, HIGH);
    } else {
      digitalWrite(dataPin, LOW);
    }
    
    digitalWrite(clkPin, HIGH);
    _delay_us(1);
    
    digitalWrite(clkPin, LOW);
    _delay_us(1);

    mask = (mask >> 1);
  }
  
  digitalWrite(dataPin, LOW);
}



void Debug2Wire::debugPrintLSb(uint8_t data) {

  // print out the byte LSb to MSb

  uint8_t mask = 0x01;
  for(int i=0; i < 8; i++) {

    if(data & mask) {
      digitalWrite(dataPin, HIGH);
    } else {
      digitalWrite(dataPin, LOW);
    }
    
    digitalWrite(clkPin, HIGH);
    _delay_us(1);
    
    digitalWrite(clkPin, LOW);
    _delay_us(1);

    mask = (mask << 1);
  }
  
  digitalWrite(dataPin, LOW);
}

