/*
  Hackvision Controller library
  Copyright (C) 2010 nootropic design, LLC
  All rights reserved.
 
  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
*/

#include "SNESController.h"
#if defined(ARDUINO) && ARDUINO >= 100
  #include "Arduino.h"
#else
  #include "WProgram.h"
#endif

#define DELAY 2


SNESController::SNESController() {
  init(DEFAULT_STROBE_PIN, DEFAULT_CLOCK_PIN, DEFAULT_DATA_PIN);
}

SNESController::SNESController(int strobePin, int clockPin, int dataPin) {
  init(strobePin, clockPin, dataPin);
}

void SNESController::init(int strobePin, int clockPin, int dataPin) {
  this->strobePin = strobePin;
  this->clockPin = clockPin;
  this->dataPin = dataPin;

  pinMode(strobePin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(dataPin, INPUT);
}

unsigned int SNESController::getState(void)
{
  unsigned int state = 0;

  // pulse the strobe pin
  digitalWrite(strobePin,HIGH);
  delayMicroseconds(DELAY);
  digitalWrite(strobePin,LOW);

  for (int i = 0; i < 16; i++) {
    delayMicroseconds(DELAY);
    state |= digitalRead(dataPin) << i;

    // pulse the clock pin
    digitalWrite(clockPin,HIGH);
    delayMicroseconds(DELAY);
    digitalWrite(clockPin,LOW);
  }
  return ~state;
}

