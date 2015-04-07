#include <avr/pgmspace.h>
#include "Wire.h"
#include "DigiKeyboard.h"
#include "StartArray.h"
#include "ProgramArray240p.h"
#include "ProgramArray480i.h"


#define GBS_I2C_ADDRESS 0x17 //0x2E

#define INTERLACED false
#define PROGRESSIVE true

char debugMessage[81];

int intProgToggleButtonPin = 5; 
bool intProgTogglePreviousValue = INTERLACED; 

int progressiveStatusLEDPin = 1;


void printDebugMessage()
{
  DigiKeyboard.sendKeyStroke(0);
  DigiKeyboard.println(debugMessage);
  DigiKeyboard.delay(100);
}


bool i2cWriteOneByte(uint8_t slaveAddress, uint8_t slaveRegister, uint8_t value)
{
  /*
    Write to One Control Register: 
    -Start Signal 
    -Slave Address Byte (R/W Bit = Low) 
    -Base Address Byte 
    -Data Byte to Base Address 
    -Stop Signal 
  */
  
  Wire.beginTransmission(slaveAddress);
  Wire.write(slaveRegister);
  Wire.write(value);
  Wire.endTransmission();
  
  return true;
}

bool i2cWriteOneByte(uint8_t slaveRegister, uint8_t value)
{
  return i2cWriteOneByte(GBS_I2C_ADDRESS, slaveRegister, value); 
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
 
  Wire.beginTransmission(slaveAddress);
  Wire.write(slaveRegister);
  
  for(int i = 0; i < numValues; i++)
  {
    Wire.write(values[i]); 
  }
  
  Wire.endTransmission();
 
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
    // REPLACE WITH I2C WRITE
    sprintf(debugMessage, "writing to register %d the value %d", pgm_read_byte(startArray + (z*2)), pgm_read_byte(startArray + (z*2) + 1) );
    printDebugMessage();
    i2cWriteOneByte(pgm_read_byte(startArray + (z*2)), pgm_read_byte(startArray + (z*2) + 1));
  }
    
  return true;
}



bool writeProgramArray(const uint8_t* programArray)
{
  sprintf(debugMessage, "Writing programArray");
  printDebugMessage();
  
  for(int y = 0; y < 6; y++)
  {
    sprintf(debugMessage, "writing to register 0x%.2x the y value %d", 0xF0, y);
    printDebugMessage();
    i2cWriteOneByte(0xF0, (uint8_t)y );
 
    for(int z = 0; z < 15; z++)
    {
      uint8_t bank[16];
      for(int w = 0; w < 16; w++)
      {
        bank[w] = pgm_read_byte(programArray + (y*256 + z*16 + w));
      }
      
      sprintf(debugMessage, "  writing bank array to register 0x%.2x", z*16);
      printDebugMessage();
      
      for(int temp=0; temp < 16; temp++)
      {
        sprintf(debugMessage, "    writing byte 0x%.2x", bank[temp]);
        printDebugMessage();
      }
      
      i2cWriteBytes(z*16, bank, 16);
    }
    
  }
  
  return true;
}


void setup() {
  
  pinMode(intProgToggleButtonPin, INPUT);
  digitalWrite(intProgToggleButtonPin, HIGH);
  
  pinMode(progressiveStatusLEDPin, OUTPUT);  
  
  sprintf(debugMessage, "Setting up Wire library");
  printDebugMessage();
  
  Wire.begin();
 
  sprintf(debugMessage, "Printing the arrays");
  printDebugMessage();
   
  writeStartArray();
}


void loop() {
  DigiKeyboard.delay(2000);
  
  bool intProgToggleState = digitalRead(intProgToggleButtonPin);
  
  if(intProgToggleState != intProgTogglePreviousValue)
  {
    // the toggle state has changed
    
    if(intProgToggleState == INTERLACED)
    {
      // INTERLACED
      writeProgramArray(programArray480i);
      digitalWrite(progressiveStatusLEDPin, LOW);
    }
    else
    {
      // PROGRESSIVE
      writeProgramArray(programArray240p);
      digitalWrite(progressiveStatusLEDPin, HIGH);
    }
    
    intProgTogglePreviousValue = intProgToggleState;
  }
  
}
