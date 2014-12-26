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
int clockPin = 5; // Digispark pin number for both SNES controllers' clock pin
int latchPin = 2; // Digispark pin number for both SNES controllers' latch pin
int dataPin1 = 0; // Digispark pin number for SNES controller 1's Data pin
int dataPin2 = 1; // Digispark pin number for SNES  ontroller 2's Data pin
uint16_t controller1Register = 0x0000; // 16 bit number to hold the 8 button values and 4 directions of SNES controller 1
uint16_t controller2Register = 0x0000; // 16 bit number to hold the 8 button values and 4 directions of SNES controller 2
uint8_t usbGameControllerRegisterLOW = 0x00;  // button status for controller 1 will eventually be stored here
uint8_t usbGameControllerRegisterHIGH = 0x00; // button status for controller 2 will eventually be stored here


void setup() {
  pinMode(clockPin,OUTPUT);    // The digispark controls the clock pin
  pinMode(latchPin,OUTPUT);    // The digispark controls the latch pin
  pinMode(dataPin1,INPUT);     // The digispark reads the value of the data pin for SNES controller 1
  digitalWrite(dataPin1,HIGH); // to enable the internal pull up resister (so buttons are off when no controller is plugged in)
  pinMode(dataPin2,INPUT); // The digispark reads the value of the data pin for SNES controller 2
  digitalWrite(dataPin2,HIGH); // to enable the internal pull up resister (so buttons are off when no controller is plugged in)
  digitalWrite(clockPin,HIGH); // clock is initially set high
  digitalWrite(latchPin,LOW);  // latch is initially set low
  
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
  }
  
  
  // Now that we have the button status for both controllers, we now set the appropriate
  // USB HID controller buttons and axis
  
  // controller 1
  usbGameControllerRegisterLOW = (char)(controller1Register & 0x000F) | (char)((controller1Register & 0x0F00) >> 4);

  // controller 1 x-axis
  if(controller1Register & 0x0040)
  {
    // left was pressed
    DigiJoystick.setX((byte) 0x00);
  }
  else if(controller1Register & 0x0080)
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
  if(controller1Register & 0x0020)
  {
    // down was pressed
    DigiJoystick.setY((byte) 0xFF); // y-axis is flipped.  FF is down, 00 is up
  }
  else if(controller1Register & 0x0010)
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
  usbGameControllerRegisterHIGH = (char)(controller2Register & 0x000F) | (char)((controller2Register & 0x0F00) >> 4);
  
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
