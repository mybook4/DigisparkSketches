/*
  Hackvision Controller library
  Copyright (C) 2010 nootropic design, LLC
  All rights reserved.
 
  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
*/

extern "C" {
  #include <stdlib.h>
  #include <string.h>
  #include <inttypes.h>
  #include "i2c.h"
}

#include <TVout.h>
#include <HardwareSerial.h>
#if defined(ARDUINO) && ARDUINO >= 100
  #include "Arduino.h"
#else
  #include "wiring.h"
#endif
#include "Nunchuk.h"

// Initialize Class Variables //////////////////////////////////////////////////


uint8_t NunchukClass::txAddress = 0;
uint8_t NunchukClass::transmitting = 0;
uint8_t nFrames;
uint8_t frameCounter = 0;
uint8_t pollNunchuk = 0;

// Constructors ////////////////////////////////////////////////////////////////

NunchukClass::NunchukClass()
{
}

void setNunchukPollFlag() {
  // This function gets called during every vertical blanking interval (once per video frame).
  // Set the flag to poll the nunchuk device every 4th frame.
  // Cannot call getData() from the callback.  Just indicates that it's kinda safe to call it.
  if (frameCounter++ >= nFrames) {
    frameCounter = 0;
    pollNunchuk = 1;
  }
}


// Public Methods //////////////////////////////////////////////////////////////

uint8_t NunchukClass::init(TVout tv, uint8_t n) {
  Nunchuk.begin();
  delay(100);
  pollNunchuk = 1;
  Nunchuk.getData();
  if ((Nunchuk.getAccelerometerX() == 0) && (Nunchuk.getAccelerometerY() == 0) && (Nunchuk.getAccelerometerZ() == 0) && (Nunchuk.getJoystickX() == 0) && (Nunchuk.getJoystickY() == 0)) {
    return 0;
  } else {
    nFrames = n;
    pollNunchuk = 0;
    tv.set_vbi_hook(&setNunchukPollFlag);
    return 1;
  }
}

void NunchukClass::begin(void)
{
  twi_init();

  beginTransmission(0x52);// transmit to device 0x52
  send(0x40);// sends memory address
  send(0x00);// sends sent a zero.  
  endTransmission();// stop transmitting
}


uint8_t NunchukClass::requestFrom(int address, int quantity)
{
  return requestFrom((uint8_t)address, (uint8_t)quantity);
}
uint8_t NunchukClass::requestFrom(uint8_t address, uint8_t quantity)
{
  // perform blocking read into buffer
  uint8_t read = twi_readFrom(address, quantity);

  return read;
}




void NunchukClass::beginTransmission(int address)
{
  beginTransmission((uint8_t)address);
}
void NunchukClass::beginTransmission(uint8_t address)
{
  // indicate that we are transmitting
  transmitting = 1;
  // set address of targeted slave
  txAddress = address;

  twi_masterBufferIndex = 0;
  twi_masterBufferLength = 0;
}


uint8_t NunchukClass::endTransmission(void)
{
  int8_t ret = twi_writeTo(txAddress, 1);
  twi_masterBufferIndex = 0;
  twi_masterBufferLength = 0;
  transmitting = 0;
  return ret;
}


void NunchukClass::send(int data)
{
  send((uint8_t)data);
}
void NunchukClass::send(uint8_t data)
{
  if (transmitting) {
    twi_masterBuffer[twi_masterBufferLength++] = data;  // use length
  }
}


// Preinstantiate Nunchuk Object
NunchukClass Nunchuk = NunchukClass();



// Nunchuk API

uint8_t* NunchukClass::getData(void) {
  if (pollNunchuk == 0) {
    return dataBuffer;
  }
  pollNunchuk = 0;
  int n = requestFrom(0x52, 6); // request data from nunchuck
  
  for(int i=0;i<n;i++) {
    dataBuffer[i] = (twi_masterBuffer[i] ^ 0x17) + 0x17;
  }
  requestData();  // send request for next data payload
  return dataBuffer;
}

int NunchukClass::getButtonC(void) {
  return ((dataBuffer[5] >> 1) & 1) ? 0 : 1;
}

int NunchukClass::getButtonZ(void) {
  return ((dataBuffer[5] >> 0) & 1) ? 0 : 1;
}

int NunchukClass::getAccelerometerX(void) {
  return dataBuffer[2];
}

int NunchukClass::getAccelerometerY(void) {
  return dataBuffer[3];
}

int NunchukClass::getAccelerometerZ(void) {
  return dataBuffer[4];
}

int NunchukClass::getJoystickX(void) {
  return dataBuffer[0];
}

int NunchukClass::getJoystickY(void) {
  return dataBuffer[1];
}

void NunchukClass::requestData()
{
    beginTransmission(0x52);// transmit to device 0x52
    send(0x00);// sends one byte
    endTransmission();// stop transmitting
}

