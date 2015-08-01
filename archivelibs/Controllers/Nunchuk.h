/*
  Hackvision Controller library
  Copyright (C) 2010 nootropic design, LLC
  All rights reserved.
 
  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
*/
#ifndef Nunchuk_h
#define Nunchuk_h

#include <inttypes.h>

static uint8_t dataBuffer[6];   // array to store nunchuck data
void setNunchukPollFlag();

class NunchukClass
{
  private:
    static uint8_t* rxBuffer;
    static uint8_t rxBufferIndex;
    static uint8_t rxBufferLength;

    static uint8_t txAddress;
    static uint8_t* txBuffer;
    static uint8_t txBufferIndex;
    static uint8_t txBufferLength;

    static uint8_t transmitting;


    void requestData();
  public:
    NunchukClass();

    uint8_t* getData();
    int getButtonC();
    int getButtonZ();
    int getJoystickX();
    int getJoystickY();
    int getAccelerometerX();
    int getAccelerometerY();
    int getAccelerometerZ();

    uint8_t init(TVout, uint8_t);
    void begin();
    void beginTransmission(uint8_t);
    void beginTransmission(int);
    uint8_t endTransmission(void);
    uint8_t requestFrom(uint8_t, uint8_t);
    uint8_t requestFrom(int, int);
    void send(uint8_t);
    void send(int);
};

extern NunchukClass Nunchuk;
extern uint8_t pollNunchuk;

#endif

