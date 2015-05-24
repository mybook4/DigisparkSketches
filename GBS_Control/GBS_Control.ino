#include <avr/pgmspace.h>
#include "I2CBitBanger.h"
#include "StartArray.h"
#include "ProgramArray240p.h"
#include "ProgramArray480i.h"
#include "NECIRReceiver.h"
#include "RemoteControlButtonValues.h"
#include <EEPROM.h>

// I2C stuff
#define GBS_I2C_ADDRESS 0x17 // 0x2E
I2CBitBanger i2cObj(GBS_I2C_ADDRESS);

// remote control stuff
#define GBS_REMOTE_DSPARK_GPIO_PIN 11
NECIRReceiver gbsRemoteControl(GBS_REMOTE_DSPARK_GPIO_PIN);

// persitent storage related items
#define VALID_BANK 0x7e
#define SCALING_SEGMENT 0x03
#define HRST_LOW  0x01
#define HRST_HIGH 0x02
#define HBST_LOW  0x04
#define HBST_HIGH 0x05
#define HBSP_LOW  0x05
#define HBSP_HIGH 0x06
#define HSCALE_LOW  0x16
#define HSCALE_HIGH 0x17
#define VRST_LOW  0x02
#define VRST_HIGH 0x03
#define VBST_LOW  0x07
#define VBST_HIGH 0x08
#define VBSP_LOW  0x08
#define VBSP_HIGH 0x09

struct SettingsBank {
  /*
   Settings banks are stored in onboard EEPROM.
   Each bank contains 10 bytes of data
   One bank consists of the following:
     byte     value
     0        validity (byte value VALID_BANK (i.e. 0x7e) if bank contains valid data.  any other value indicates the bank should be ignored)
     1        horizontal shift low
     2        horizontal shift med
     3        horizontal shift high
     4        horizontal scale low
     5        horizontal scale high
     6        vertical shift low
     7        veritcal shift med
     8        vertical shift high
     9        reserved (value currently undefined)
 
  uint8_t valid;
  uint8_t hShiftLow;
  uint8_t hShiftMed;
  uint8_t hShiftHigh;
  uint8_t hScaleLow;
  uint8_t hScaleHigh;
  uint8_t vShiftLow;
  uint8_t vShiftMed;
  uint8_t vShiftHigh;
  uint8_t reserved;
  */
  uint8_t data[10];
};


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
  if(readFromRegister(SCALING_SEGMENT, HRST_LOW, 1, &hrstLow) != 1) {
    return;
  }
  
  if(readFromRegister(HRST_HIGH, 1, &hrstHigh) != 1) {
    return;
  }
  
  hrstValue = ( ( ((uint16_t)hrstHigh) & 0x0007) << 8) | (uint16_t)hrstLow;
  
  // get HBST
  if(readFromRegister(HBST_LOW, 1, &hbstLow) != 1) {
    return;
  }
  
  if(readFromRegister(HBST_HIGH, 1, &hbstHigh) != 1) {
    return;
  }
  
  hbstValue = ( ( ((uint16_t)hbstHigh) & 0x000f) << 8) | (uint16_t)hbstLow;
  
  // get HBSP
  hbspLow = hbstHigh;
  
  if(readFromRegister(HBSP_HIGH, 1, &hbspHigh) != 1) {
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
  
  writeOneByte(HBST_LOW, (uint8_t)(hbstValue & 0x00ff));
  writeOneByte(HBST_HIGH, ((uint8_t)(hbspValue & 0x000f) << 4) | ((uint8_t)((hbstValue & 0x0f00) >> 8)) );
  writeOneByte(HBSP_HIGH, (uint8_t)((hbspValue & 0x0ff0) >> 4) );  
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
  
  if(readFromRegister(SCALING_SEGMENT, HSCALE_LOW, 1, &low) != 1) {
    return;
  }
  
  if(readFromRegister(HSCALE_HIGH, 1, &high) != 1) {
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
  
  writeOneByte(HSCALE_LOW, newLow);
  writeOneByte(HSCALE_HIGH, newHigh);
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
  if(readFromRegister(SCALING_SEGMENT, VRST_LOW, 1, &vrstLow) != 1) {
    return;
  }
  
  if(readFromRegister(VRST_HIGH, 1, &vrstHigh) != 1) {
    return;
  }
  
  vrstValue = ( (((uint16_t)vrstHigh) & 0x007f) << 4) | ( (((uint16_t)vrstLow) & 0x00f0) >> 4);
  
  // get VBST
  if(readFromRegister(VBST_LOW, 1, &vbstLow) != 1) {
    return;
  }
  
  if(readFromRegister(VBST_HIGH, 1, &vbstHigh) != 1) {
    return;
  }
  
  vbstValue = ( ( ((uint16_t)vbstHigh) & 0x0007) << 8) | (uint16_t)vbstLow;
  
  // get VBSP
  vbspLow = vbstHigh;
  
  if(readFromRegister(VBSP_HIGH, 1, &vbspHigh) != 1) {
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
  
  writeOneByte(VBST_LOW, (uint8_t)(vbstValue & 0x00ff));
  writeOneByte(VBST_HIGH, ((uint8_t)(vbspValue & 0x000f) << 4) | ((uint8_t)((vbstValue & 0x0700) >> 8)) );
  writeOneByte(VBSP_HIGH, (uint8_t)((vbspValue & 0x07f0) >> 4) );
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


void saveCurrentSettingsToBank(uint16_t bankNumber) {
  // save the current scalar settings
  
  /*
     One bank consists of the following:
     byte     value
     0        validity (byte value VALID_BANK (i.e. 0x7e) if bank contains valid data.  any other value indicates the bank should be ignored)
     1        horizontal shift low
     2        horizontal shift med
     3        horizontal shift high
     4        horizontal scale low
     5        horizontal scale high
     6        vertical shift low
     7        veritcal shift med
     8        vertical shift high
     9        reserved (value currently undefined)
  */
  
  SettingsBank newSave;
  newSave.data[0] = VALID_BANK;
  newSave.data[9] = 0x00;
  
  if(readFromRegister(SCALING_SEGMENT, HBST_LOW, 1, &newSave.data[1]) != 1) {
    return;
  }
  
  if(readFromRegister(HBST_HIGH, 1, &newSave.data[2]) != 1) {
    return;
  }
  
  if(readFromRegister(HBSP_HIGH, 1, &newSave.data[3]) != 1) {
    return;
  }
  
  if(readFromRegister(HSCALE_LOW, 1, &newSave.data[4]) != 1) {
    return;
  }
  
  if(readFromRegister(HSCALE_HIGH, 1, &newSave.data[5]) != 1) {
    return;
  }
  
  newSave.data[5] &= 0x03; // we don't need to save surrounding data
  
  if(readFromRegister(VBST_LOW, 1, &newSave.data[6]) != 1) {
    return;
  }
  
  if(readFromRegister(VBST_HIGH, 1, &newSave.data[7]) != 1) {
    return;
  }
  
  if(readFromRegister(VBSP_HIGH, 1, &newSave.data[8]) != 1) {
    return;
  }
  
  uint16_t address = 0 + (sizeof(SettingsBank) * bankNumber);
  
  for(int i=0; i < sizeof(SettingsBank); i++) {
    EEPROM.write(address + i, newSave.data[i]);
  }
  
  return;
}

void saveCurrentSettings() {
  // Once the user has pressed the 200+ button,
  // we wait for a selection to be made (button 1-9).
  // When a selection is made, the current settings are
  // saved to the onboard EEPROM.
  // The button 100+ can cancel the request to save the current settings
  
  // save the current state of the LED
  //uint8_t existingLedPort = LED_PORT;
  
  LED_PORT &= ~(1<<LED_BIT); // turn off LED
  delay(250);
  LED_PORT |= (1<<LED_BIT); // turn on LED
  delay(250);
  LED_PORT &= ~(1<<LED_BIT); // turn off LED
  delay(250);
  LED_PORT |= (1<<LED_BIT); // turn on LED
  
  bool finished = false;
  
  while(!finished) {
    
    int remoteControlButton = gbsRemoteControl.getIRButtonValue();
    switch(remoteControlButton) {
      
      case BUTTON_1: // Save
        saveCurrentSettingsToBank(1);
        finished = true;
        break;
      
      case BUTTON_2: // Save
        saveCurrentSettingsToBank(2);
        finished = true;
        break;
      
      case BUTTON_3: // Save
        saveCurrentSettingsToBank(3);
        finished = true;
        break;
      
      case BUTTON_4: // Save
        saveCurrentSettingsToBank(4);
        finished = true;
        break;
      
      case BUTTON_5: // Save
        saveCurrentSettingsToBank(5);
        finished = true;
        break;
    
      case BUTTON_6: // Save
        saveCurrentSettingsToBank(6);
        finished = true;
        break;
      
      case BUTTON_7: // Save
        saveCurrentSettingsToBank(7);
        finished = true;
        break;
      
      case BUTTON_8: // Save
        saveCurrentSettingsToBank(8);
        finished = true;
        break;
      
      case BUTTON_9: // Save
        saveCurrentSettingsToBank(9);
        finished = true;
        break;
      
      case BUTTON_100_PLUS: // Cancel save request
        finished = true;
        break;
      
      default:
        break;
    } // switch
  } // loop
  
  LED_PORT &= ~(1<<LED_BIT); // turn off LED
  delay(250);
  LED_PORT |= (1<<LED_BIT); // turn on LED
  delay(250);
  LED_PORT &= ~(1<<LED_BIT); // turn off LED
  delay(250);
  LED_PORT |= (1<<LED_BIT); // turn on LED
  delay(250);
  LED_PORT &= ~(1<<LED_BIT); // turn off LED
  
  return;
}


void loadStoredSettings(uint8_t bankNumber) {  
  uint16_t address = 0 + (sizeof(SettingsBank) * bankNumber);
  
  SettingsBank setting;
  
  // fetch the stored settings
  for(int i=0; i < sizeof(SettingsBank); i++) {
    setting.data[i] = EEPROM.read(address + i); 
  }
  
  // check if the settings bank is valid
  if(setting.data[0] != VALID_BANK) {
    return;
  }
  
  // write the settings to the scalar chip over i2c
  /*
     One bank consists of the following:
     byte     value
     0        validity (byte value VALID_BANK (i.e. 0x7e) if bank contains valid data.  any other value indicates the bank should be ignored)
     1        horizontal shift low
     2        horizontal shift med
     3        horizontal shift high
     4        horizontal scale low
     5        horizontal scale high
     6        vertical shift low
     7        veritcal shift med
     8        vertical shift high
     9        reserved (value currently undefined)
  */
  
  uint8_t existingHscaleHigh = 0x00;
  if(readFromRegister(SCALING_SEGMENT, HSCALE_HIGH, 1, &existingHscaleHigh) != 1) {
    return;
  }
  
  writeOneByte(HBST_LOW, setting.data[1]);
  writeOneByte(HBST_HIGH, setting.data[2]);
  writeOneByte(HBSP_HIGH, setting.data[3]);
  writeOneByte(HSCALE_LOW, setting.data[4]);
  
  uint8_t newHscaleHigh = (existingHscaleHigh & 0xfc) | ((setting.data[5]) & 0x03);
  writeOneByte(HSCALE_HIGH, newHscaleHigh);
  
  writeOneByte(VBST_LOW, setting.data[6]);
  writeOneByte(VBST_HIGH, setting.data[7]);
  writeOneByte(VBSP_HIGH, setting.data[8]);
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
      
    case BUTTON_1: // Load the saved setting
      loadStoredSettings(1); 
      break;
      
    case BUTTON_2: // Load the saved setting
      loadStoredSettings(2); 
      break;
    
    case BUTTON_3: // Load the saved setting
      loadStoredSettings(3); 
      break;
    
    case BUTTON_4: // Load the saved setting
      loadStoredSettings(4); 
      break;
      
    case BUTTON_5: // Load the saved setting
      loadStoredSettings(5); 
      break;
      
    case BUTTON_6: // Load the saved setting
      loadStoredSettings(6); 
      break;
      
    case BUTTON_7: // Load the saved setting
      loadStoredSettings(7); 
      break;
      
    case BUTTON_8: // Load the saved setting
      loadStoredSettings(8); 
      break;
      
    case BUTTON_9: // Load the saved setting
      loadStoredSettings(9); 
      break;
      
    case BUTTON_200_PLUS: // Used to save current settings
      saveCurrentSettings(); 
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
