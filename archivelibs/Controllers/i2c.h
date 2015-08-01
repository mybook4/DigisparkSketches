/*
  Hackvision Controller library
  Copyright (C) 2010 nootropic design, LLC
  All rights reserved.
 
  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
*/

#ifndef i2c_h
#define i2c_h

  #include <inttypes.h>

  //#define ATMEGA8

  #ifndef CPU_FREQ
  #define CPU_FREQ 16000000L
  #endif

  #ifndef TWI_FREQ
  #define TWI_FREQ 100000L
  #endif

  #ifndef TWI_BUFFER_LENGTH
  #define TWI_BUFFER_LENGTH 8
  #endif

  #define TWI_READY 0
  #define TWI_MRX   1
  #define TWI_MTX   2
  #define TWI_SRX   3
  #define TWI_STX   4
  
  void twi_init(void);
  uint8_t twi_readFrom(uint8_t, uint8_t);
  uint8_t twi_writeTo(uint8_t, uint8_t);
  void twi_reply(uint8_t);
  void twi_stop(void);
  void twi_releaseBus(void);

  extern uint8_t* twi_masterBuffer;
  extern volatile uint8_t twi_masterBufferIndex;
  extern uint8_t twi_masterBufferLength;
  extern volatile uint8_t twi_error;

#endif

