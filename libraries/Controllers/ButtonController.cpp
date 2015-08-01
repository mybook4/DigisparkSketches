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

#include "ButtonController.h"


ButtonController::ButtonController() {
  init(3, 2, 4, 5, 10); 
}

ButtonController::ButtonController(uint8_t left, uint8_t right, uint8_t up, uint8_t down, uint8_t fire) {
  init(left, right, up, down, fire);
}


void ButtonController::init(uint8_t left, uint8_t right, uint8_t up, uint8_t down, uint8_t fire) {
  this->leftPin = left;
  this->rightPin = right;
  this->upPin = up;
  this->downPin = down;
  this->firePin = fire;

  pinMode(leftPin, INPUT);
  pinMode(rightPin, INPUT);
  pinMode(upPin, INPUT);
  pinMode(downPin, INPUT);
  pinMode(firePin, INPUT);
  digitalWrite(leftPin, HIGH);
  digitalWrite(rightPin, HIGH);
  digitalWrite(upPin, HIGH);
  digitalWrite(downPin, HIGH);
  digitalWrite(firePin, HIGH);
}


uint8_t ButtonController::leftPressed(void) {
  if (digitalRead(leftPin) == LOW) {
    return 1;
  } else {
    return 0;
  }
}

uint8_t ButtonController::rightPressed(void) {
  if (digitalRead(rightPin) == LOW) {
    return 1;
  } else {
    return 0;
  }
}

uint8_t ButtonController::upPressed(void) {
  if (digitalRead(upPin) == LOW) {
    return 1;
  } else {
    return 0;
  }
}

uint8_t ButtonController::downPressed(void) {
  if (digitalRead(downPin) == LOW) {
    return 1;
  } else {
    return 0;
  }
}

uint8_t ButtonController::firePressed(void) {
  if (digitalRead(firePin) == LOW) {
    return 1;
  } else {
    return 0;
  }
}


// Preinstantiate default ButtonController object for the integrated controller
ButtonController Controller = ButtonController();

