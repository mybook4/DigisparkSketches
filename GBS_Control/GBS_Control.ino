#include <avr/pgmspace.h>
#include "I2CBitBangWriter.h"
#include "StartArray.h"
#include "ProgramArray240p.h"
#include "ProgramArray480i.h"

#define GBS_I2C_ADDRESS 0x17 // 0x2E


bool writeOneByte(uint8_t slaveRegister, uint8_t value)
{
  return writeBytes(slaveRegister, &value, 1); 
}


bool writeBytes(uint8_t slaveAddress, uint8_t slaveRegister, uint8_t* values, uint8_t numValues)
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
 
  I2CBitBangWriter.setSlaveAddress(slaveAddress);
  
  I2CBitBangWriter.addByteForTransmission(slaveRegister);
  
  I2CBitBangWriter.addBytesForTransmission(values, numValues);
  
  if(!I2CBitBangWriter.transmitData())
  {    
    return false;
  }
 
  return true;
}


bool writeBytes(uint8_t slaveRegister, uint8_t* values, int numValues)
{
  return writeBytes(GBS_I2C_ADDRESS, slaveRegister, values, numValues);
}


bool writeStartArray()
{
  for(int z = 0; z < ((sizeof(startArray)/2) - 1520); z++)
  {
    writeOneByte(pgm_read_byte(startArray + (z*2)), pgm_read_byte(startArray + (z*2) + 1));
    delay(10);
  }
    
  return true;
}



bool writeProgramArray(const uint8_t* programArray)
{ 
  for(int y = 0; y < 6; y++)
  { 
    writeOneByte(0xF0, (uint8_t)y );
    delay(10);
 
    for(int z = 0; z < 15; z++)
    {
      uint8_t bank[16];
      for(int w = 0; w < 16; w++)
      {
        bank[w] = pgm_read_byte(programArray + (y*256 + z*16 + w));
      }
      
      writeBytes(z*16, bank, 16);
      
      delay(10);
    }
    
  }
  
  return true;
}

// Macros for a "Resolution" switch which allows for toggling between
// 240p (1) and 480i (0)
//
// NOTE: The switch schematic below has an RC debouncer connected to it for stability
//
// The presence of this switch is optional.  The 50ms delay in the loop could be enough to mask the bounce of your switch.
// WARNING: Depending on switch/button connection, you may need to enable internal pull up resistors for pins.  This would 
//
//                                  
// Digispark VCC -----/\/\/\/----------| SPDT switch |------------/\/\/\/-----GND
//                       R1                  |                      R2        |
//                                           |                                |
//                                           |----------------------||--------|           
//                                           |                      C1
//                                           |
//                                  Digispark Pro Input
//                                     GPIO 5 (PA7)
//
// R1, R2, and C1 values depend on your switch's debounce time

#define RESOLUTION_SWITCH_DDR  DDRA
#define RESOLUTION_SWITCH_PIN  PINA
#define RESOLUTION_SWITCH_PORT PORTA
#define RESOLUTION_SWITCH_BIT  7

// This variable holds the last known state of the Resolution switch
// true = 240p, false = 480i
bool lastKnownResolutionSwitchState = true;


// Digispark onboard LED macros
#define LED_DDR DDRB
#define LED_PORT PORTB
#define LED_BIT 1


void setup() {
  
  // We setup and read the value of the Resolution switch
  // to determine if we are initially going 
  // to write 240p or 480i settings to the scaler chip
  RESOLUTION_SWITCH_DDR &= ~(1<<RESOLUTION_SWITCH_BIT); // Set the resolution switch to an input
  RESOLUTION_SWITCH_PORT &= ~(1<<RESOLUTION_SWITCH_BIT); // Turn off internal pullup (we already have resistors in the debounce circuit).  PULL UPs MAY NEED TO BE ENABLED FOR YOUR CIRCUIT
  bool currentResolutionSwitchState = ((RESOLUTION_SWITCH_PIN & (1<<RESOLUTION_SWITCH_BIT)) != 0); // Read the value of the Resolution switch
  
  // NOTE: If a resolution switch isn't wanted, simply comment out the three code lines above and uncomment one the following lines
  //bool currentResolutionSwitchState = true; // 240p
  //bool currentResolutionSwitchState = false; // 480i
  
  
  // We setup the LED to tell show switch state
  LED_DDR |= (1<<LED_BIT);  
  
  I2CBitBangWriter.initialize();
   
  // Write the start array
  writeStartArray();
  
  if(currentResolutionSwitchState == true) {
    // Write the 240p register values to the scaler chip
    writeProgramArray(programArray240p);
    LED_PORT |= (1<<LED_BIT); // turn on LED
  } else {
    // Write the 480i register values to the scaler chip
    writeProgramArray(programArray480i);
    LED_PORT &= ~(1<<LED_BIT); // turn off LED
  }
  
  lastKnownResolutionSwitchState = currentResolutionSwitchState;
}


void loop() {
  delay(50); // delay 50 milliseconds

  // Read the state of the Resolution switch
  bool currentResolutionSwitchState = ((RESOLUTION_SWITCH_PIN & (1<<RESOLUTION_SWITCH_BIT)) != 0);
  
  if(currentResolutionSwitchState != lastKnownResolutionSwitchState)
  {
    // The switch state has been flipped
    // Write the appropriate register values to the scaler chip
    
    if(currentResolutionSwitchState == true) {
      // Write the 240p register values to the scaler chip
      writeProgramArray(programArray240p);
      LED_PORT |= (1<<LED_BIT); // turn on LED
    } else {
      // Write the 480i register values to the scaler chip
      writeProgramArray(programArray480i);
      LED_PORT &= ~(1<<LED_BIT); // turn off LE
    }
    
    lastKnownResolutionSwitchState = currentResolutionSwitchState;
  }

}
