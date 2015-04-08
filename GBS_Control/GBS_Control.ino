#include <avr/pgmspace.h>
#include "TinyWireM.h"
#include "DigiKeyboard.h"
#include "StartArray.h"
#include "ProgramArray240p.h"
#include "ProgramArray480i.h"


#define GBS_I2C_ADDRESS 0x17 // 0x2E

char debugMessage[81];


void printDebugMessage()
{
  DigiKeyboard.sendKeyStroke(0);
  DigiKeyboard.println(debugMessage);
  DigiKeyboard.delay(100);
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
 
  //sprintf(debugMessage, "i2cWriteBytes");
  //printDebugMessage();
 
  TinyWireM.beginTransmission(slaveAddress);
  
  TinyWireM.send(slaveRegister);
  
  for(int i = 0; i < numValues; i++)
  {
    TinyWireM.send(values[i]);
  }
  
  byte endRet = TinyWireM.endTransmission();
  if(endRet != 0)
  {
    sprintf(debugMessage, "Error: End Trans ret %d", endRet);
    printDebugMessage();
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
  sprintf(debugMessage, "startArray");
  printDebugMessage();

  
  for(int z = 0; z < ((sizeof(startArray)/2) - 1520); z++)
  {    
    //sprintf(debugMessage, "writing to register %d the value %d", pgm_read_byte(startArray + (z*2)), pgm_read_byte(startArray + (z*2) + 1) );
    //printDebugMessage();
    i2cWriteOneByte(pgm_read_byte(startArray + (z*2)), pgm_read_byte(startArray + (z*2) + 1));
    DigiKeyboard.delay(10);
  }
    
  return true;
}



bool writeProgramArray(const uint8_t* programArray)
{
  sprintf(debugMessage, "programArray");
  printDebugMessage();
  
  for(int y = 0; y < 6; y++)
  {
    sprintf(debugMessage, "Register segment %d", y);
    printDebugMessage();
    
    i2cWriteOneByte(0xF0, (uint8_t)y );
    DigiKeyboard.delay(10);
 
    for(int z = 0; z < 15; z++)
    {
      uint8_t bank[16];
      for(int w = 0; w < 16; w++)
      {
        bank[w] = pgm_read_byte(programArray + (y*256 + z*16 + w));
      }
      
      i2cWriteBytes(z*16, bank, 16);
      DigiKeyboard.delay(10);
    }
    
  }
  
  return true;
}


void setup() {
  sprintf(debugMessage, "TinyWireM.begin()");
  printDebugMessage();
  
  TinyWireM.begin();
   
  // Write the start array
  writeStartArray();
  
  // Write the 240p register values for now
  // TODO: Create small UI with available GPIO, buttons, and LEDs to choose between 240p and 480i
  writeProgramArray(programArray240p);
}


void loop() {
  sprintf(debugMessage, "idle in loop");
  printDebugMessage();
  
  DigiKeyboard.delay(5000);
}
