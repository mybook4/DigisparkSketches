#include <avr/pgmspace.h>
#include "I2CBitBanger.h"
#include "StartArray.h"
#include "ProgramArray240p.h"
#include "ProgramArray480i.h"
#include "NECIRReceiver.h"
#include "RemoteControlButtonValues.h"

// I2C stuff
#define GBS_I2C_ADDRESS 0x17 // 0x2E
I2CBitBanger i2cObj(GBS_I2C_ADDRESS);

// remote control stuff
#define GBS_REMOTE_DSPARK_GPIO_PIN 11
NECIRReceiver gbsRemoteControl(GBS_REMOTE_DSPARK_GPIO_PIN);

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
 
  //i2cObj.setSlaveAddress(slaveAddress);
  
  i2cObj.addByteForTransmission(slaveRegister);
  
  i2cObj.addBytesForTransmission(values, numValues);
  
  if(!i2cObj.transmitData())
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
    delay(1);
  }
    
  return true;
}



bool writeProgramArray(const uint8_t* programArray)
{ 
  for(int y = 0; y < 6; y++)
  { 
    writeOneByte(0xF0, (uint8_t)y );
    delay(1);
 
    for(int z = 0; z < 15; z++)
    {
      uint8_t bank[16];
      for(int w = 0; w < 16; w++)
      {
        bank[w] = pgm_read_byte(programArray + (y*256 + z*16 + w));
      }
      
      writeBytes(z*16, bank, 16);
      
      delay(1);
    }
    
  }
  
  return true;
}


int readFromRegister(uint8_t segment, uint8_t reg, int bytesToRead, uint8_t* output) {
  
  // go to the appropriate segment
  if(!writeOneByte(0xF0, segment)) {
    return 0;
  }
  
  return readFromRegister(reg, bytesToRead, output);
}

int readFromRegister(uint8_t reg, int bytesToRead, uint8_t* output) {
    
  // go to the appropriate register
  i2cObj.addByteForTransmission(reg);
  i2cObj.transmitData();
  
  return i2cObj.recvData(bytesToRead, output);
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

void returnToDefaultSettings() {
  writeProgramArray(programArray480i); //
  LED_PORT &= ~(1<<LED_BIT); // turn off LED
}

void shiftHorizontal(uint16_t amountToAdd, bool subtracting) {
  
  uint8_t hrstLow = 0x00;
  uint8_t hrstHigh = 0x00;
  uint16_t hrstValue = 0x0000;
  uint8_t hbstLow = 0x00;
  uint8_t hbstHigh = 0x00;
  uint16_t hbstValue = 0x0000;
  uint8_t hbspLow = 0x00;
  uint8_t hbspHigh = 0x00;
  uint16_t hbspValue = 0x0000;
  
  // get HRST
  if(readFromRegister(0x03, 0x01, 1, &hrstLow) != 1) {
    return;
  }
  
  if(readFromRegister(0x02, 1, &hrstHigh) != 1) {
    return;
  }
  
  hrstValue = ( ( ((uint16_t)hrstHigh) & 0x0007) << 8) | (uint16_t)hrstLow;
  
  // get HBST
  if(readFromRegister(0x04, 1, &hbstLow) != 1) {
    return;
  }
  
  if(readFromRegister(0x05, 1, &hbstHigh) != 1) {
    return;
  }
  
  hbstValue = ( ( ((uint16_t)hbstHigh) & 0x000f) << 8) | (uint16_t)hbstLow;
  
  // get HBSP
  hbspLow = hbstHigh;
  
  if(readFromRegister(0x06, 1, &hbspHigh) != 1) {
    return;
  }
  
  hbspValue = ( ( ((uint16_t)hbspHigh) & 0x00ff) << 4) | ( (((uint16_t)hbspLow) & 0x00f0) >> 4);
  
  // Perform the addition/subtraction
  if(subtracting) {
    hbstValue -= amountToAdd;
    hbspValue -= amountToAdd;
  } else {
    hbstValue += amountToAdd;
    hbspValue += amountToAdd;
  }
  
  // handle the case where hbst or hbsp have been decremented below 0
  if(hbstValue & 0x8000) {
    hbstValue = hrstValue-1;
  }
  
  if(hbspValue & 0x8000) {
    hbspValue = hrstValue-1;
  }
  
  writeOneByte(0x04, (uint8_t)(hbstValue & 0x00ff));
  writeOneByte(0x05, ((uint8_t)(hbspValue & 0x000f) << 4) | ((uint8_t)((hbstValue & 0x0f00) >> 8)) );
  writeOneByte(0x06, (uint8_t)((hbspValue & 0x0ff0) >> 4) );  
}


void shiftHorizontalLeft() {
  shiftHorizontal(4, true);
}

void shiftHorizontalRight() {
  shiftHorizontal(4, false);
}

void scaleHorizontal(uint16_t amountToAdd, bool subtracting) {
  uint8_t high = 0x00;
  uint8_t newHigh = 0x00;
  uint8_t low = 0x00;
  uint8_t newLow = 0x00;
  uint16_t newValue = 0x0000;
  
  if(readFromRegister(0x03, 0x16, 1, &low) != 1) {
    return;
  }
  
  if(readFromRegister(0x17, 1, &high) != 1) {
    return;
  }
  
  newValue = ( ( ((uint16_t)high) & 0x0003) * 256) + (uint16_t)low;
  
  if(subtracting) {
    newValue -= amountToAdd;
  } else {
    newValue += amountToAdd;
  }
  
  newHigh = (high & 0xfc) | (uint8_t)( (newValue / 256) & 0x0003);
  newLow = (uint8_t)(newValue & 0x00ff);
  
  writeOneByte(0x16, newLow);
  writeOneByte(0x17, newHigh);
}

void scaleHorizontalSmaller() {
  scaleHorizontal(4, false);
}

void scaleHorizontalLarger() {
  scaleHorizontal(4, true);
}


void shiftVertical(uint16_t amountToAdd, bool subtracting) {
  
  uint8_t vrstLow = 0x00;
  uint8_t vrstHigh = 0x00;
  uint16_t vrstValue = 0x0000;
  uint8_t vbstLow = 0x00;
  uint8_t vbstHigh = 0x00;
  uint16_t vbstValue = 0x0000;
  uint8_t vbspLow = 0x00;
  uint8_t vbspHigh = 0x00;
  uint16_t vbspValue = 0x0000;
  
  // get VRST
  if(readFromRegister(0x03, 0x02, 1, &vrstLow) != 1) {
    return;
  }
  
  if(readFromRegister(0x03, 1, &vrstHigh) != 1) {
    return;
  }
  
  vrstValue = ( (((uint16_t)vrstHigh) & 0x007f) << 4) | ( (((uint16_t)vrstLow) & 0x00f0) >> 4);
  
  // get VBST
  if(readFromRegister(0x07, 1, &vbstLow) != 1) {
    return;
  }
  
  if(readFromRegister(0x08, 1, &vbstHigh) != 1) {
    return;
  }
  
  vbstValue = ( ( ((uint16_t)vbstHigh) & 0x0007) << 8) | (uint16_t)vbstLow;
  
  // get VBSP
  vbspLow = vbstHigh;
  
  if(readFromRegister(0x09, 1, &vbspHigh) != 1) {
    return;
  }
  
  vbspValue = ( ( ((uint16_t)vbspHigh) & 0x007f) << 4) | ( (((uint16_t)vbspLow) & 0x00f0) >> 4);
  
  // Perform the addition/subtraction
  if(subtracting) {
    vbstValue -= amountToAdd;
    vbspValue -= amountToAdd;
  } else {
    vbstValue += amountToAdd;
    vbspValue += amountToAdd;
  }
  
  // handle the case where hbst or hbsp have been decremented below 0
  if(vbstValue & 0x8000) {
    vbstValue = vrstValue-1;
  }
  
  if(vbspValue & 0x8000) {
    vbspValue = vrstValue-1;
  }
  
  writeOneByte(0x07, (uint8_t)(vbstValue & 0x00ff));
  writeOneByte(0x08, ((uint8_t)(vbspValue & 0x000f) << 4) | ((uint8_t)((vbstValue & 0x0700) >> 8)) );
  writeOneByte(0x09, (uint8_t)((vbspValue & 0x07f0) >> 4) );
}


void shiftVerticalUp() {
  shiftVertical(4, true);
}

void shiftVerticalDown() {
  shiftVertical(4, false);
}



void debugWithLED(int numBytes, uint8_t* buf) {

      for(int i = 0; i < numBytes; i++) {
        
      LED_PORT &= ~(1<<LED_BIT); // turn off LED
      delay(1000);
      
      LED_PORT |= (1<<LED_BIT);  // turn on LED
      delay(5000);
      
      LED_PORT &= ~(1<<LED_BIT); // turn off LED
      delay(1000);
  
      uint8_t mask = 0x80;
      for(int j=0; j < 8; j++) {
        if(buf[i] & mask) {
          // blink twice
          LED_PORT |= (1<<LED_BIT);  // turn on LED
          delay(500);
          LED_PORT &= ~(1<<LED_BIT); // turn off LED
          delay(500);
          LED_PORT |= (1<<LED_BIT);  // turn on LED
          delay(500);
          LED_PORT &= ~(1<<LED_BIT); // turn off LED
          delay(500);
          
        } else {
          // blink once
          LED_PORT |= (1<<LED_BIT);  // turn on LED
          delay(500);
          LED_PORT &= ~(1<<LED_BIT); // turn off LED
          delay(500);
        }
        mask = mask >> 1; 
        delay(3000);
      }
      
    }
    
}


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
  
  // Remote control stuff
  pinMode(GBS_REMOTE_DSPARK_GPIO_PIN, INPUT);
  
   
  // Write the start array
  writeStartArray();
  
  /* OLD SWITCH CODE
  if(currentResolutionSwitchState == true) {
    // Write the 240p register values to the scaler chip
    writeProgramArray(programArray240p);
    LED_PORT |= (1<<LED_BIT); // turn on LED
  } else {
    // Write the 480i register values to the scaler chip
    //writeProgramArray(programArray480i); // original set file values by dooklink
    writeProgramArray(programArray480i); //
    LED_PORT &= ~(1<<LED_BIT); // turn off LED
  }

  lastKnownResolutionSwitchState = currentResolutionSwitchState;
  */
  
  returnToDefaultSettings();
}


void loop() {
    
  int remoteControlButton = gbsRemoteControl.getIRButtonValue();
  switch(remoteControlButton) {
      
    case BUTTON_CH_MINUS: // Change to 480i default mode
      writeProgramArray(programArray480i); //
      LED_PORT &= ~(1<<LED_BIT); // turn off LED
      break;
      
    case BUTTON_CH_PLUS:  // Change to 240p default mode
      writeProgramArray(programArray240p);
      LED_PORT |= (1<<LED_BIT); // turn on LED
      break;
          
    case BUTTON_CH:// Return to default settings
      returnToDefaultSettings();
      break;
    
    case BUTTON_PREV: // Horizontal shift left 
      shiftHorizontalLeft();
      break;
      
    case BUTTON_NEXT: // Horizontal shift right 
      shiftHorizontalRight();
      break;
      
    case BUTTON_VOL_MINUS: // Horizontal scale smaller 
      scaleHorizontalSmaller();
      break;
      
    case BUTTON_VOL_PLUS: // Horizontal scall larger 
      scaleHorizontalLarger();
      break;
      
    case BUTTON_PLAY_PAUSE: // Vertical shift up 
      shiftVerticalUp();
      break;
      
    case BUTTON_EQ: // Vertical shift down
      shiftVerticalDown(); 
      break;
    
    case -1: 
      break;  // Unrecognized input.  Don't change any settings.  This also ignores held down button sequences
    
    default:
      break;
  }
    
  
  /* OLD SWITCH CODE
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
      //writeProgramArray(programArray480i); // original set values by dooklink
      writeProgramArray(programArray480i); //
      LED_PORT &= ~(1<<LED_BIT); // turn off LE
    }
    
    lastKnownResolutionSwitchState = currentResolutionSwitchState;
  }
  */

}
