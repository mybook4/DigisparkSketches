//DigiJoystick test and usage documentation

//Good Documentation of the SNES Controller (German) http://www.rn-wissen.de/index.php/SNES_Controller
#include "DigiJoystick.h"

/* Some data about the SNES controller protocol from http://www.gamefaqs.com/snes/916396-super-nintendo/faqs/5395

SNES Controller pinout

       ----------------------------- ---------------------
      |                             |                      \
      | (1)     (2)     (3)     (4) |   (5)     (6)     (7) |
      |                             |                      /
       ----------------------------- ---------------------


        Pin     Description 
        ===     =========== 
        1       +5v         
        2       Data clock   
        3       Data latch    
        4       Serial data     
        5       ?              
        6       ?               
        7       Ground      


SNES Controller Communication Protocol

                           |<------------16.67ms------------>|

                           12us
                        -->|   |<--

                            ---                               ---
                           |   |                             |   |
        Data Latch      ---     -----------------/ /----------    
--------...


        Data Clock      ----------   -   -   -  -/ /----------------   -  
...
                                  | | | | | | | |                   | | | |
                                   -   -   -   -                     -   -
                                   1   2   3   4                     1   2

        Serial Data         ----     ---     ----/ /           ---
                           |    |   |   |   |                 |
        (Buttons B      ---      ---     ---        ----------
        & Select        norm      B      SEL           norm
        pressed).       low                            low
                                12us
                             -->|   |<--




        Clock Cycle     Button Reported
        ===========     ===============
        1               B
        2               Y
        3               Select
        4               Start
        5               Up on joypad
        6               Down on joypad
        7               Left on joypad
        8               Right on joypad
        9               A
        10              X
        11              L
        12              R
        13              none (always high)
        14              none (always high)
        15              none (always high)
        16              none (always high)


For two SNES controllers to be connected to the digispark (rev 2 model), the following connection/wiring scheme was used:

SNES Controller 1 Pin   SNES Controller 2 Pin    Digispark Pin
=====================   =====================    =============
1                       1                        5V
2                       2                        P2
3                       3                        P5
4                                                P0*
                        4                        P1*
7                       7                        GND


*NOTE: For my rev 2 digispark, I cut out the LED attached 
       to pin P1 (so as to not interfere with the SNES data 
       communications).  Those with rev B digispark would
       cut out the LED attached to pin P0.

When connected to a PC, the controller adapter will show up as one 6 axis, 16 button controller.
The X/Y axis and buttons 1 through 8 correspond to the first SNES controller.
The SLIDER/ZROT and buttons 9 through 16 correspond to the second SNES controller.
The button mapping is as follows (corresponds to the order in which buttons are clocked out of the SNES controller):

Button 1 => SNES B
Button 2 => SNES Y
Button 3 => SNES Select
Button 4 => SNES Start
Button 5 => SNES A
Button 6 => SNES X
Button 7 => SNES L
Button 8 => SNES R


*/

boolean pinState = false; 
int clockPin = 5; // Digispark Pro pin number for both SNES controllers' clock pin
int latchPin = 2; // Digispark Pro pin number for both SNES controllers' latch pin
int dataPin1 = 0; // Digispark Pro pin number for SNES controller 1's Data pin
int dataPin2 = 12; // Digispark Pro pin number for SNES  ontroller 2's Data pin

uint16_t controller1Register = 0x0000; // 16 bit number to hold the 8 button values and 4 directions of SNES controller 1
uint16_t controller2Register = 0x0000; // 16 bit number to hold the 8 button values and 4 directions of SNES controller 2
uint8_t usbGameControllerRegisterLOW = 0x00;  // button status for controller 1 will eventually be stored here
uint8_t usbGameControllerRegisterHIGH = 0x00; // button status for controller 2 will eventually be stored here

boolean pinStateSaturn = false;
int satS0 = 6; // Digispark Pro pin number for the Saturn digital controller's Select 0 pin
int satS1 = 7; // Digispark Pro pin number for the Saturn digital controller's Select 1 pin
int satD0 = 8; // Digispark Pro pin number for the Saturn digital controller's Data 0 pin 
int satD1 = 9; // Digispark Pro pin number for the Saturn digital controller's Data 1 pin
int satD2 = 10; // Digispark Pro pin number for the Saturn digital controller's Data 2 pin
int satD3 = 11; // Digispark Pro pin number for the Saturn digital controller's Data 3 pin
uint16_t satControllerRegister = 0x0000; // 16 bit number to hold the 9 button values and 4 directions of the Saturn digital controller

#define SATURN_A_BUTTON_BIT_MASK     0x0010
#define SATURN_B_BUTTON_BIT_MASK     0x0001
#define SATURN_C_BUTTON_BIT_MASK     0x0080
#define SATURN_X_BUTTON_BIT_MASK     0x0020
#define SATURN_Y_BUTTON_BIT_MASK     0x0002
#define SATURN_Z_BUTTON_BIT_MASK     0x0040
#define SATURN_L_BUTTON_BIT_MASK     0x0004
#define SATURN_R_BUTTON_BIT_MASK     0x0400
#define SATURN_START_BUTTON_BIT_MASK 0x0008
#define SATURN_UP_BUTTON_BIT_MASK    0x1000
#define SATURN_DOWN_BUTTON_BIT_MASK  0x2000
#define SATURN_LEFT_BUTTON_BIT_MASK  0x4000
#define SATURN_RIGHT_BUTTON_BIT_MASK 0x8000

void setup() {
  // SNES
  pinMode(clockPin,OUTPUT);    // The digispark controls the clock pin
  pinMode(latchPin,OUTPUT);    // The digispark controls the latch pin
  pinMode(dataPin1,INPUT);     // The digispark reads the value of the data pin for SNES controller 1
  digitalWrite(dataPin1,HIGH); // to enable the internal pull up resister (so buttons are off when no controller is plugged in)
  pinMode(dataPin2,INPUT); // The digispark reads the value of the data pin for SNES controller 2
  digitalWrite(dataPin2,HIGH); // to enable the internal pull up resister (so buttons are off when no controller is plugged in)
  
  digitalWrite(clockPin,HIGH); // clock is initially set high
  digitalWrite(latchPin,LOW);  // latch is initially set low
  
  // Saturn digital
  pinMode(satS0,OUTPUT); // The digispark controls the S0 pin
  pinMode(satS1,OUTPUT); // The digispark controls the S1 pin
  pinMode(satD0,INPUT);  // The digispark reads the value of the D0 pin for the Saturn digital controller
  digitalWrite(satD0,HIGH); // to enable internal pull up resistor (so buttons are off when no controller is plugged in)
  pinMode(satD1,INPUT);  // The digispark reads the value of the D1 pin for the Saturn digital controller
  digitalWrite(satD1,HIGH); // to enable internal pull up resistor (so buttons are off when no controller is plugged in)
  pinMode(satD2,INPUT);  // The digispark reads the value of the D2 pin for the Saturn digital controller
  digitalWrite(satD2,HIGH); // to enable internal pull up resistor (so buttons are off when no controller is plugged in)
  pinMode(satD3,INPUT);  // The digispark reads the value of the D3 pin for the Saturn digital controller
  digitalWrite(satD3,HIGH); // to enable internal pull up resistor (so buttons are off when no controller is plugged in)
  
  digitalWrite(satS0,LOW); // initially, S0 is low
  digitalWrite(satS1,LOW); // initially, S1 is low

  
  // All axis are set to the neutral position
  DigiJoystick.setX((byte) 0x80);
  DigiJoystick.setY((byte) 0x80);      
  DigiJoystick.setXROT((byte) 0x80);
  DigiJoystick.setYROT((byte) 0x80);
  DigiJoystick.setZROT((byte) 0x80);
  DigiJoystick.setSLIDER((byte) 0x80);
}


void loop() {
 
  // Initalize both SNES controllers by setting latch high for 12 uSeconds
  digitalWrite(latchPin, HIGH);
  delayMicroseconds(12);
  digitalWrite(latchPin, LOW);
  
  // Initialize the Saturn select lines
  digitalWrite(satS0,LOW); // initially, S0 is low
  digitalWrite(satS1,LOW); // initially, S1 is low

  // We first read the controller button status into our 16 bit controller registers
  for(int i=0; i<16; i++) {
    delayMicroseconds(6);
    digitalWrite(clockPin, LOW);
    
    // read and store the button status for controller 1
    pinState=!digitalRead(dataPin1);
    if(pinState){
      controller1Register |= (0x01<<i);
    }else{
      controller1Register &= ~(0x01<<i);
    }
    
    // read and store the button status for controller 2
    pinState=!digitalRead(dataPin2);
    if(pinState){
      controller2Register |= (0x01<<i);
    }else{
      controller2Register &= ~(0x01<<i);
    }

    delayMicroseconds(6);
    digitalWrite(clockPin, HIGH);
    
    // Saturn logic
    if(i == 0) // (S1, S0) = 0,0
    {
      // we already initialized the select lines to 0,0 (S1, S0), so we read and store
      // D0 = Z, D1 = Y, D2 = X, D3 = R
      pinStateSaturn = digitalRead(satD0);
      if(pinStateSaturn)
      {
        satControllerRegister |= SATURN_Z_BUTTON_BIT_MASK;
      }
      else
      {
        satControllerRegister &= ~(SATURN_Z_BUTTON_BIT_MASK);
      }
      
      pinStateSaturn = digitalRead(satD1);
      if(pinStateSaturn)
      {
        satControllerRegister |= SATURN_Y_BUTTON_BIT_MASK;
      }
      else
      {
        satControllerRegister &= ~(SATURN_Y_BUTTON_BIT_MASK);
      }
      
      pinStateSaturn = digitalRead(satD2);
      if(pinStateSaturn)
      {
        satControllerRegister |= SATURN_X_BUTTON_BIT_MASK;
      }
      else
      {
        satControllerRegister &= ~(SATURN_X_BUTTON_BIT_MASK);
      }
      
      pinStateSaturn = digitalRead(satD3);
      if(pinStateSaturn)
      {
        satControllerRegister |= SATURN_R_BUTTON_BIT_MASK;
      }
      else
      {
        satControllerRegister &= ~(SATURN_R_BUTTON_BIT_MASK);
      }
      
      // update the select lines for the next read
      digitalWrite(satS0,HIGH);
      digitalWrite(satS1,LOW);
    }
    else if(i == 4) // (S1, S0) = 0,1
    {
      // read button states
      // D0 = B, D1 = C, D2 = A, D3 = Start
      pinStateSaturn = digitalRead(satD0);
      if(pinStateSaturn)
      {
        satControllerRegister |= SATURN_B_BUTTON_BIT_MASK;
      }
      else
      {
        satControllerRegister &= ~(SATURN_B_BUTTON_BIT_MASK);
      }
      
      pinStateSaturn = digitalRead(satD1);
      if(pinStateSaturn)
      {
        satControllerRegister |= SATURN_C_BUTTON_BIT_MASK;
      }
      else
      {
        satControllerRegister &= ~(SATURN_C_BUTTON_BIT_MASK);
      }
      
      pinStateSaturn = digitalRead(satD2);
      if(pinStateSaturn)
      {
        satControllerRegister |= SATURN_A_BUTTON_BIT_MASK;
      }
      else
      {
        satControllerRegister &= ~(SATURN_A_BUTTON_BIT_MASK);
      }
      
      pinStateSaturn = digitalRead(satD3);
      if(pinStateSaturn)
      {
        satControllerRegister |= SATURN_START_BUTTON_BIT_MASK;
      }
      else
      {
        satControllerRegister &= ~(SATURN_START_BUTTON_BIT_MASK);
      }
      

      
      // update the select lines for the next read
      digitalWrite(satS0,LOW);
      digitalWrite(satS1,HIGH);
    }
    else if(i == 8) // (S1, S0) = 1,0
    {
      // read button states
      // D0 = Up, D1 = Down, D2 = Left, D3 = Right
      pinStateSaturn = digitalRead(satD0);
      if(pinStateSaturn)
      {
        satControllerRegister |= SATURN_UP_BUTTON_BIT_MASK;
      }
      else
      {
        satControllerRegister &= ~(SATURN_UP_BUTTON_BIT_MASK);
      }
      
      pinStateSaturn = digitalRead(satD1);
      if(pinStateSaturn)
      {
        satControllerRegister |= SATURN_DOWN_BUTTON_BIT_MASK;
      }
      else
      {
        satControllerRegister &= ~(SATURN_DOWN_BUTTON_BIT_MASK);
      }
      
      pinStateSaturn = digitalRead(satD2);
      if(pinStateSaturn)
      {
        satControllerRegister |= SATURN_LEFT_BUTTON_BIT_MASK;
      }
      else
      {
        satControllerRegister &= ~(SATURN_LEFT_BUTTON_BIT_MASK);
      }
      
      pinStateSaturn = digitalRead(satD3);
      if(pinStateSaturn)
      {
        satControllerRegister |= SATURN_RIGHT_BUTTON_BIT_MASK;
      }
      else
      {
        satControllerRegister &= ~(SATURN_RIGHT_BUTTON_BIT_MASK);
      }
      
      // update the select lines for the next read
      digitalWrite(satS0,HIGH);
      digitalWrite(satS1,HIGH);
    }
    else if(i == 12) // (S1, S0) = 1,1
    {
      // read button states
      // D0 = don't care, D1 = don't care, D2 = don't care, D3 = L
      pinStateSaturn = digitalRead(satD3);
      if(pinStateSaturn)
      {
        satControllerRegister |= SATURN_L_BUTTON_BIT_MASK;
      }
      else
      {
        satControllerRegister &= ~(SATURN_L_BUTTON_BIT_MASK);
      }
      
      // the select lines will be updated to 0,0 (S1, S0) when loop() is run again
    }
  }
  
  
  // Now that we have the button status for both controllers, we now set the appropriate
  // USB HID controller buttons and axis
  
  // controller 1
  usbGameControllerRegisterLOW = (uint8_t)(controller1Register & 0x000F) | (uint8_t)((controller1Register & 0x0F00) >> 4);
  
  // we OR in the Saturn controller bits
  usbGameControllerRegisterLOW |= (uint8_t)(satControllerRegister & 0x00FF);

  // controller 1 x-axis
  if( (/*SNES*/controller1Register & 0x0040) || (/*Saturn*/satControllerRegister & SATURN_LEFT_BUTTON_BIT_MASK) )
  {
    // left was pressed
    DigiJoystick.setX((byte) 0x00);
  }
  else if( (/*SNES*/controller1Register & 0x0080) || (/*Saturn*/satControllerRegister & SATURN_RIGHT_BUTTON_BIT_MASK) )
  {
    // right was pressed 
    DigiJoystick.setX((byte) 0xFF);
  }
  else
  {
    // D-Pad is centered in the X direction 
    DigiJoystick.setX((byte) 0x80);
  }
  
  // controller 1 y-axis
  if( (/*SNES*/controller1Register & 0x0020) || (/*Saturn*/satControllerRegister & SATURN_DOWN_BUTTON_BIT_MASK) ) 
  {
    // down was pressed
    DigiJoystick.setY((byte) 0xFF); // y-axis is flipped.  FF is down, 00 is up
  }
  else if( (/*SNES*/controller1Register & 0x0010) || (/*Saturn*/satControllerRegister & SATURN_UP_BUTTON_BIT_MASK) )
  {
    // up was pressed 
    DigiJoystick.setY((byte) 0x00);
  }
  else
  {
    // D-Pad is centered in the Y direction 
    DigiJoystick.setY((byte) 0x80);
  }
  
  
  
  // controller 2
  usbGameControllerRegisterHIGH = (uint8_t)(controller2Register & 0x000F) | (uint8_t)((controller2Register & 0x0F00) >> 4);
  
  // we OR in the Saturn controller bits.  Since the Saturn digital controller has 9 action buttons, one button overflows into the second SNES controller)
  usbGameControllerRegisterHIGH |= (uint8_t)( (satControllerRegister >> 8) & SATURN_R_BUTTON_BIT_MASK);
  
  // controller 2 x-axis
  if(controller2Register & 0x0040)
  {
    // left was pressed
    DigiJoystick.setSLIDER((byte) 0x00);
  }
  else if(controller2Register & 0x0080)
  {
    // right was pressed 
    DigiJoystick.setSLIDER((byte) 0xFF);
  }
  else
  {
    // D-Pad is centered in the X direction 
    DigiJoystick.setSLIDER((byte) 0x80);
  }
  
  // controller 2 y-axis
  if(controller2Register & 0x0020)
  {
    // down was pressed
    DigiJoystick.setZROT((byte) 0x00);
  }
  else if(controller2Register & 0x0010)
  {
    // up was pressed 
    DigiJoystick.setZROT((byte) 0xFF);
  }
  else
  {
    // D-Pad is centered in the Y direction 
    DigiJoystick.setZROT((byte) 0x80);
  }
  
  
  
  DigiJoystick.setButtons((char) usbGameControllerRegisterLOW , (char) usbGameControllerRegisterHIGH);
  DigiJoystick.delay(14); // used to be 16. changed to 14 to compensate for extra instructions performed when translating controller registers to USB gamepad buttons/axis

}
