/*
  Hackvision Controller library
  Copyright (C) 2010 nootropic design, LLC
  All rights reserved.
 
  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
*/

#ifndef ButtonController_h
#define ButtonController_h

#include <inttypes.h>
#if defined(ARDUINO) && ARDUINO >= 100
  #include "Arduino.h"
#else
  #include "WProgram.h"
  #include <pins_arduino.h>
#endif

class ButtonController {
 private:
  void init(uint8_t left, uint8_t right, uint8_t up, uint8_t down, uint8_t fire);
  uint8_t leftPin;
  uint8_t rightPin;
  uint8_t upPin;
  uint8_t downPin;
  uint8_t firePin;

 public:
    ButtonController();
    ButtonController(uint8_t left, uint8_t right, uint8_t up, uint8_t down, uint8_t fire);
  
    uint8_t leftPressed();
    uint8_t rightPressed();
    uint8_t upPressed();
    uint8_t downPressed();
    uint8_t firePressed();
};

extern ButtonController Controller;

#endif

