/*
  Hackvision Controller library
  Copyright (C) 2010 nootropic design, LLC
  All rights reserved.
 
  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
*/
#ifndef Paddle_h
#define Paddle_h

#include <inttypes.h>
#if defined(ARDUINO) && ARDUINO >= 100
  #include "Arduino.h"
#else
  #include "WProgram.h"
  #include <pins_arduino.h>
#endif

class Paddle {
 private:
  void init(uint8_t button, uint8_t potentiometer);
  uint8_t buttonPin;
  uint8_t potPin;

 public:
    Paddle(uint8_t button, uint8_t potentiometer);
  
    uint8_t buttonPressed();
    int getPosition();
};

extern Paddle PaddleA;
extern Paddle PaddleB;

#endif

