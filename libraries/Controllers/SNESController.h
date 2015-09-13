/*
  Hackvision Controller library
  Copyright (C) 2010 nootropic design, LLC
  All rights reserved.
 
  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
*/

#ifndef SNESController_h
#define SNESController_h

#include <inttypes.h>

/*
#if defined(ARDUINO) && ARDUINO >= 100
  #include "Arduino.h"
#else
  #include "WProgram.h"
  #include <pins_arduino.h>
#endif
*/

#define SNES_B       0x01
#define SNES_Y       0x02
#define SNES_SELECT  0x04
#define SNES_START   0x08
#define SNES_UP      0x10
#define SNES_DOWN    0x20
#define SNES_LEFT    0x40
#define SNES_RIGHT   0x80
#define SNES_A       0x100
#define SNES_X       0x200
#define SNES_L       0x400
#define SNES_R       0x800

class SNESController {
 public:
  SNESController();
  SNESController(int strobePin, int clockPin, int dataPin);
  unsigned int getState();

 private:
  void init(int strobePin, int clockPin, int dataPin);
  int strobePin;
  int clockPin;
  int dataPin;
};


#define DEFAULT_STROBE_PIN 6
#define DEFAULT_CLOCK_PIN 8
#define DEFAULT_DATA_PIN 13


#endif /* SNESController_h */

