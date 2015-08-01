/*
  Hackvision Controller library
  Copyright (C) 2010 nootropic design, LLC
  All rights reserved.
 
  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
*/

#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include "Paddle.h"


Paddle::Paddle(uint8_t button, uint8_t potentiometer) {
  init(button, potentiometer);
}


void Paddle::init(uint8_t button, uint8_t potentiometer) {
  this->buttonPin = button;
  this->potPin = potentiometer;

  pinMode(buttonPin, INPUT);
  digitalWrite(buttonPin, HIGH);
}

uint8_t Paddle::buttonPressed(void) {
  if (digitalRead(buttonPin) == LOW) {
    return 1;
  } else {
    return 0;
  }
}

int Paddle::getPosition(void) {
  return analogRead(potPin);
}


// Preinstantiate default Paddle objects
// I need to think about whether this is the right thing to do.  Anyone using the
// controllers library will have the pins configured automatically.  
Paddle PaddleA = Paddle(0, 3);
Paddle PaddleB = Paddle(1, 2);

