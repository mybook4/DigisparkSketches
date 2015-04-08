#include <avr/pgmspace.h>
#include "TinyWireM.h"
#include "StartArray.h"
#include "ProgramArray240p.h"
#include "ProgramArray480i.h"

#define DEBUG_PRINT

#ifdef DEBUG_PRINT
#include "DigiKeyboard.h"
#endif

#define GBS_I2C_ADDRESS 0x17 // 0x2E

char debugMessage[81];


void printDebugMessage()
{
  #ifdef DEBUG_PRINT
  DigiKeyboard.sendKeyStroke(0);
  DigiKeyboard.println(debugMessage);
  DigiKeyboard.delay(100);
  #endif
}


bool i2cWriteOneByte(uint8_t slaveRegister, uint8_t value)
{
  return i2cWriteBytes(slaveRegister, &value, 1); 
}


bool i2cWriteBytes(uint8_t slaveAddress, uint8_t slaveRegister, uint8_t* values, int numValues)
{
  /*
   Write to Consecutive Control Registers:
    -Start Signal 
    -Slave Address Byte (R/W Bit = Low) 
    -Base Address Byte 
    -Data Byte to Base Address 
    -Data Byte to (Base Address + 1) 
    -Data Byte to (Base Address + 2) 
    -Data Byte to (Base Address + 3) 
    .................. 
    -Stop Signal
  */
 
  TinyWireM.beginTransmission(slaveAddress);
  
  TinyWireM.send(slaveRegister);
  
  for(int i = 0; i < numValues; i++)
  {
    TinyWireM.send(values[i]);
  }
  
  byte endRet = TinyWireM.endTransmission();
  if(endRet != 0)
  {
    #ifdef DEBUG_PRINT
    sprintf(debugMessage, "Error: End Trans ret %d", endRet);
    printDebugMessage();
    #endif
    
    return false;
  }
 
  return true;
}


bool i2cWriteBytes(uint8_t slaveRegister, uint8_t* values, int numValues)
{
  return i2cWriteBytes(GBS_I2C_ADDRESS, slaveRegister, values, numValues);
}


bool writeStartArray()
{
  #ifdef DEBUG_PRINT
  sprintf(debugMessage, "startArray");
  printDebugMessage();
  #endif
  
  for(int z = 0; z < ((sizeof(startArray)/2) - 1520); z++)
  {    
    i2cWriteOneByte(pgm_read_byte(startArray + (z*2)), pgm_read_byte(startArray + (z*2) + 1));
    
    #ifdef DEBUG_PRINT
    DigiKeyboard.delay(10);
    #else
    delay(10);
    #endif
  }
    
  return true;
}



bool writeProgramArray(const uint8_t* programArray)
{
  #ifdef DEBUG_PRINT
  sprintf(debugMessage, "programArray");
  printDebugMessage();
  #endif
  
  for(int y = 0; y < 6; y++)
  {
    #ifdef DEBUG_PRINT
    sprintf(debugMessage, "Register segment %d", y);
    printDebugMessage();
    #endif
    
    i2cWriteOneByte(0xF0, (uint8_t)y );
    
    #ifdef DEBUG_PRINT
    DigiKeyboard.delay(10);
    #else
    delay(10);
    #endif
 
    for(int z = 0; z < 15; z++)
    {
      uint8_t bank[16];
      for(int w = 0; w < 16; w++)
      {
        bank[w] = pgm_read_byte(programArray + (y*256 + z*16 + w));
      }
      
      i2cWriteBytes(z*16, bank, 16);
      
      #ifdef DEBUG_PRINT
      DigiKeyboard.delay(10);
      #else
      delay(10);
      #endif
    }
    
  }
  
  return true;
}


void setup() {
  #ifdef DEBUG_PRINT
  sprintf(debugMessage, "TinyWireM.begin()");
  printDebugMessage();
  #endif
  
  TinyWireM.begin();
   
  // Write the start array
  writeStartArray();
  
  // Write the 240p register values for now
  // TODO: Create small UI with available GPIO, buttons, and LEDs to choose between 240p and 480i
  writeProgramArray(programArray240p);
}


void loop() {
  #ifdef DEBUG_PRINT
  sprintf(debugMessage, "idle in loop");
  printDebugMessage();
  DigiKeyboard.delay(5000);
  #else
  delay(5000);
  #endif
}
