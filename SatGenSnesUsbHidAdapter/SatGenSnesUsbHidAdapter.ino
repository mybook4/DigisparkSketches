/*


=========================================== Intro ===========================================
The following sketch implements the logic for a controller adapter 
that translates between Saturn (digital), Genesis (3-button), or SNES, 
and a USB HID game controller.

The code was written for a Digispark Pro (ATTiny167).



==================== Genesis Controller Pinout/Protocol Information =========================
Only the standard 3-button Genesis controller is supported at this time (6-button wasn't
felt necessary due to the support for the Saturn controller, which have 6 face buttons).

The Genesis controller uses a standard DB9 connector.  The controller connector is female, 
and the console connector is male.  The Genesis controller houses a 74HC157 Quad 2-input MUX.


Controller connector (female):

  ------------- 
5 \ * * * * * / 1
 9 \ * * * * / 6
    ---------

Console connector (male):

  ------------- 
1 \ * * * * * / 5
 6 \ * * * * / 9
    ---------


Pinout and Logic:

Pin    Connection
-----------------
1      Controller Output (Up)              - D0
2      Controller Output (Down)            - D1
3      Controller MUX Output (Gnd/Left)    - D4
4      Controller MUX Output (Gnd/Right)   - D5
5      Supply (+5 Vdc)
6      Controller MUX Output (A/B)         - D2
7      Controller Input (MUX Select Signal)- Sel
8      Supply Ground (0 Vdc)
9      Controller MUX Output (Start/C)     - D3


Pin    Signal (Select=Gnd)    Signal (Select=5V)
-------------------------------------------------
1      Up                     Up
2      Down                   Down
3      Gnd                    Left
4      Gnd                    Right
5      Supply 5 Vdc           Supply 5 Vdc
6      Button A               Button B
7      Select                 Select
8      Supply Ground          Supply Ground
9      Button Start           Button C

Sel|  D0   D1   D2   D3   D4   D5
------------------------------------
0  |  Up   Dn   A    Strt 0    0
1  |  Up   Dn   B    C    Lft  Rgt

All Dx lines are active low (button pressed --> 0v).
All Dx lines are inactive High (button released --> 5v).


The specific male DB9 connector (from a scrapped PC) used for the adapter had a ribbon cable as follows:

Wire     DB9 Pin    Genesis Connection
---------------------------------------
red      1          D0
grey     6          D2
grey     2          D1
grey     7          Sel
grey     3          D4
grey     8          0v
grey     4          D5
grey     9          D3
grey     5          5v




===================== Saturn Controller Pinout/Protocol Information =========================
Only the Saturn digital controller is supported at this time (it wasn't felt necessary to
support the Analog controller since I don't have one).

The Saturn controller uses a proprietary 9 pin connector.
The controller uses two 74XX153 (e.g. 74HC153) Dual 4-input multiplexer ICs to mux 13 signals
(from up to 16 possible signals).


Controller connector (male):

       -------------------
      /                  / \
   9 /                1 /  /|
    -------------------/  / |
   / * * * * * * * * * \ / / 
  |                     | /
  -----------------------/


Console connector (female):

   1                9
    -------------------
   /                   \
  |  * * * * * * * * *  |
  -----------------------


Pinout and Logic:

A Saturn controller extention cable was used (the colors in the table are specific to that
cable).

Ext Cable   Pin  Signal
------------------------
blue         1   Supply (+5 Vdc)
green        2   D1 (Controller Output)
black        3   D0 (Controller Output)
orange       4   S0 (Controller Input, MUX select 0)
red          5   S1 (Controller Input, MUX select 1)
brown        6   Not Used in this Adapter
yellow       7   D3 (Controller Output)
grey         8   D2 (Controller Output)
white        9   Supply Ground (0 Vdc)



S1  S0  |  D0    D1    D2    D3
-----------------------------------
0   0   |  Z     Y     X     R
0   1   |  B     C     A     Start
1   0   |  Up    Down  Left  Right
1   1   |  *     *     *     L

'*' = don't care

All Dx lines are active low (button pressed --> 0v).
All Dx lines are inactive High (button released --> 5v).


====================== SNES Controller Pinout/Protocol Information ==========================
The SNES uses a proprietary 7-pin connector.

The controller acts as a parallel load serial shift register.



Controller connector (female and male):

       ----------------------------- ---------------------
      |                             |                      \
      | (1)     (2)     (3)     (4) |   (5)     (6)     (7) |
      |                             |                      /
       ----------------------------- ---------------------


Pinout and Logic:

A SNES controller extention cable was used (the colors in the table are specific to that
cable).

Ext Cable    Pin    Connection
-----------------
green        1      Supply (+5 Vdc)
blue         2      Clock (Controller Input)
yellow       3      Latch (Controller Input)
red          4      Data  (Controller Output)
             5      Not Used in this Adapter
             6      Not Used in this Adapter
black        7      Supply Ground (0 Vdc)


There are 4 directions and 8 buttons on the SNES controller, but the controller protocol can
clock out 16 bits (B, Y, Select, Start, Up, Down, Left, Right, A, X, L, R, 1, 1, 1, 1).


              12us
              ____
Latch   _____|    |___________________________________________________________________

        _____________    __    __       __    __    __    __    __    __    __________
Clock                |__|  |__|  |_..._|  |__|  |__|  |__|  |__|  |__|  |__| 
                   6  6  6us   
Data    ______________________________________________________________________________         
                  |B    |Y    |Sel ... |L    |R    |1    |1    |1    |1    |       
                  ----------------------------------------------------------

The controller shifts places button state on Data on the rising clock edge (except for 
button B, which is placed on the falling edge of Latch).
The console reads the state of Data on the falling clock edge.
Note that the last four "buttons" are optional to read.


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




================================== Circuit Design Approach ==================================
To keep the adapter simple and efficiently re-use pins on the limited ATTiny167 I/O, only 
one of the controllers will be able to be plugged in at a time.  

NOTE!!!:  A Saturn and Genesis controller should NEVER be plugged in at the same time.
If they are, the controller ICs (74HC157 in the Genesis controller and 74HC153s in the 
Saturn conroller) could be damaged (see schematic below).  This is a result of D0-D3 being 
directly connected between the Saturn and Genesis conenctors (i.e. if a given Dx for each
controller were driven to opposite values, e.g. 0v and 5v simultaneously).

Ideally, this would be resolved via the addition of a basic logic IC, but I did not have 
one on-hand (e.g. a single Quad AND gate (e.g. 74HC08).


Saturn Connector                 Genesis Connector      ATTiny167            SNES                   
----------------                 -----------------     -----------          ------
                                                    
Pin1___(5v)______________________Pin5__(5v)_____________+5V__________________Pin1__(5v)

Pin2___(D1)______________________Pin2__(D1)_____________PA1

Pin3___(D0)______________________Pin1__(D0)_____________PA0
                
Pin4___(S0)______________________Pin7__(Sel)____________PA6
                
Pin5___(S1)_____________________________________________PA7                 
              
Pin6__x                          Pin3__(D4)_____________PA4
             
Pin7___(D3)______________________Pin9__(D3)_____________PA3
             
Pin8___(D2)______________________Pin6__(D2)_____________PA2
             
                                 Pin4__(D5)_____________PA5

                                                        PB0__________________Pin4__(Data)

                                                        PB1__________________Pin3__(Latch)

                                                        PB2__________________Pin2__(Clock)

Pin9___(0v)______________________Pin8__(0v)_____________0v___________________Pin7__(0v)

             
               



All ATTiny167 inputs have the internal pullups enabled.



Note that it was specifically chosen to have Genesis pins 3 and 4 (D4 and D5)
connect to their own ATTiny167 inputs.  This was done as a means to detect, on
the fly, whether a Genesis or Saturn controller is plugged in.  The Saturn 
conroller has 4 data lines from its multiplexers and the Genesis controller 
has 6 data lines.  Since internal pullups are enabled on the ATTiny167, if a 
Genesis controller is not present, the associated ATTiny167 inputs for D4 and D5 
will both be pulled to 5v.  If a Genesis controller is present, D4 and D5 will 
be 0v when Sel is set to 0v.  At that point, the program can adjust its logic
to ensure the correct buttons are mapped to the correct USB controller buttons.

When a Saturn or Genesis controller is plugged in, initally all Dx lines are 5v.
(with the exception of the Genesis controller D4 and D5 when Sel is 0v).
When a button is pressed, the associated Dx line is 0v.



================================== Program Algorithm Overview ===============================
The approach for the SNES controller is to mimick the timing diagram in the section above.

The approach for the Saturn/Genesis controller is to do the following in a loop (after reading
SNES)

1)  Set S0/Sel to 0(0v).

2)  Read the value of D4 and D5.
    If D4 and D5 are 0(0v), then a Genesis Controller is plugged in.  See 3a.
    If D4 and D5 are 1(5v), then a Saturn Controller (or nothing) is plugged in.  See 3b.
    The case where D4 and D5 are not the same shouldn't occur, but treat it as Saturn.  See 3b.

3a) Read the values of D0-D3.  Store (interpreting as Genesis).  Don't store D4 and D5 (still 0v).
    Set S0/Sel to 1(5v).
    Read the values of D0-D5.  Store (interpreting as Genesis).
   
3b) Read the values of D0-D3.  Store (interpreting as Saturn). (arrived in [S1,S0] [0,0])
    Set S0/Sel to 1(5v). (now in [0,1])
    Read the values of D0-D3.  Store (interpreting as Saturn).
    Set S1 to 1(5v).     (now in [1,1])
    Read the values of D0-D3.  Store (interpreting as Saturn).
    Set S0/Sel to 0(0v). (now in [1,0])
    Read the values of D0-D3.  Store (interpreting as Saturn).
    Set S1 to 0(0v). (now in [0,0])


When connected to a PC, the controller adapter will show up as one 6 axis, 16 button controller.

The button mapping is as follows:

USB HID     SNES     Saturn    Genesis
----------------------------------------------------------------------
-x          Left     Left      Left  (Y when A+B+C pressed for either Saturn or Genesis)
+x          Right    Right     Right (A when A+B+C pressed for either Saturn or Genesis)
-y          Down     Down      Down  (B when A+B+C pressed for either Saturn or Genesis)
+y          Up       Up        Up    (X when A+B+C pressed for either Saturn or Genesis)
Button 0    B        B         B
Button 1    Y        A         A
Button 2    Select   A+B+C     A+B+C (disables directions and A,B,C)
Button 3    Start    Start     Start
Button 4    A        C         C
Button 5    X        Y         
Button 6    L        X         
Button 7    R        Z         
Button 8
Button 9             L
Button 10            R
Button 11
Button 12
Button 13
Button 14
Button 15


*/







#include "DigiJoystick.h"


uint8_t usbGameControllerRegisterLOW = 0x00;  // button status for will eventually be stored here
uint8_t usbGameControllerRegisterHIGH = 0x00; // button status for will eventually be stored here


// SNES controller pins
uint16_t snesControllerRegister = 0x0000; // 16 bit number to hold the 8 button values and 4 directions of the SNES controller

// Snes port definitions
#define SNES_PIN  PINB
#define SNES_PORT PORTB
#define SNES_DDR  DDRB
#define SNES_DATA  PB0 // Digispark Pro pin number for SNES controller's Data pin
#define SNES_LATCH PB1 // Digispark Pro pin number for SNES controller's Latch pin
#define SNES_CLOCK PB2 // Digispark Pro pin number for SNES controller's Clock pin

#define SNES_UP_BUTTON_MASK    0x0010
#define SNES_DOWN_BUTTON_MASK  0x0020
#define SNES_LEFT_BUTTON_MASK  0x0040
#define SNES_RIGHT_BUTTON_MASK 0x0080

#define SNES_START_BUTTON_MASK_POST_SHIFT      0x0008
#define SNES_SELECT_BUTTON_MASK_POST_SHIFT     0x0004
#define SNES_Y_BUTTON_MASK_POST_SHIFT          0x0002
#define SNES_B_BUTTON_MASK_POST_SHIFT          0x0001

#define SNES_A_BUTTON_MASK_POST_SHIFT     0x0010
#define SNES_X_BUTTON_MASK_POST_SHIFT     0x0020
#define SNES_L_BUTTON_MASK_POST_SHIFT     0x0040
#define SNES_R_BUTTON_MASK_POST_SHIFT     0x0080


// Saturn controller pins
uint16_t satControllerRegister = 0x0000; // 16 bit number to hold the 9 button values and 4 directions of Saturn controller

// Saturn port definitions
#define SAT_PIN  PINA
#define SAT_PORT PORTA
#define SAT_DDR  DDRA
#define SAT_S0   PA6
#define SAT_S1   PA7
#define SAT_D0   PA0
#define SAT_D1   PA1
#define SAT_D2   PA2
#define SAT_D3   PA3

#define SAT_A_BUTTON_MASK     0x0002
#define SAT_B_BUTTON_MASK     0x0001
#define SAT_C_BUTTON_MASK     0x0010
#define SAT_X_BUTTON_MASK     0x0040
#define SAT_Y_BUTTON_MASK     0x0020
#define SAT_Z_BUTTON_MASK     0x0080
#define SAT_L_BUTTON_MASK     0x0200
#define SAT_R_BUTTON_MASK     0x0400
#define SAT_START_BUTTON_MASK 0x0008
#define SAT_UP_BUTTON_MASK    0x1000
#define SAT_DOWN_BUTTON_MASK  0x2000
#define SAT_LEFT_BUTTON_MASK  0x4000
#define SAT_RIGHT_BUTTON_MASK 0x8000


/*
After SNES shift
R L X A St Se Y B

Corresponding Sat (updated 2022/01/16)
Select is a placeholder, it is derived from the values of A+B+C, not an actual button
Z X Y C St _ A B
*/



// Genesis controller pins
uint16_t genControllerRegister = 0x0000; // 16 bit number to hold the 4 button values and 4 directions of Genesis controller

// Genesis port definitions
#define GEN_PIN  PINA
#define GEN_PORT PORTA
#define GEN_DDR  DDRA
#define GEN_SEL  PA6
#define GEN_D0   PA0
#define GEN_D1   PA1
#define GEN_D2   PA2
#define GEN_D3   PA3
#define GEN_D4   PA4
#define GEN_D5   PA5

#define GEN_UP_BUTTON_MASK        0x1000
#define GEN_DOWN_BUTTON_MASK      0x2000
#define GEN_LEFT_BUTTON_MASK      0x4000
#define GEN_RIGHT_BUTTON_MASK     0x8000
#define GEN_A_BUTTON_MASK         0x0002
#define GEN_B_BUTTON_MASK         0x0001
#define GEN_C_BUTTON_MASK         0x0010
#define GEN_START_BUTTON_MASK     0x0008


/**
 * Reads the value of a button and stores into a controller register based on a button mask
 *
 * @param controllerRegister The controller register where the button value will be stored (pressed=1, unpressed=0)
 * @param pinRegister The AVR PIN register used to read the button values from (e.g. PINA)
 * @param pinPosition The position within the AVR PIN register for the read
 * @param buttonMask The buttonMask that will be used to store button state into controllerRegister
 * @param lowIsPressed When true, a 0 will be interpretted as a button press and a 1 will be interpretted as unpressed
 *
 **/
void readStoreButtonState(uint16_t& controllerRegister, uint8_t pinRegister, uint8_t pinPosition, uint16_t buttonMask, bool lowIsPressed=false) {

  if(lowIsPressed) {
    if(pinRegister & (1<<pinPosition)) {
      controllerRegister &= ~(buttonMask);
    } else {
      controllerRegister |= buttonMask;
    }

  } else {
    if(pinRegister & (1<<pinPosition)) {
      controllerRegister |= buttonMask;
    } else {
      controllerRegister &= ~(buttonMask);
    }
  }

}



void setup() {

  // Set up SNES lines
  // NOTE: We don't set the entire SNES DDR to 0x00 as USB uses (part of the port (PB3 and PB6)
  SNES_DDR  &= ~(1<<SNES_DATA); // set Data as an input pin
  SNES_PORT |=  (1<<SNES_DATA); // enable the Data line internal pullup resistor
  SNES_DDR  |=  (1<<SNES_LATCH) | (1<<SNES_CLOCK); // set Latch and Clock as outputs

  SNES_PORT |= (1<<SNES_CLOCK);  // clock is set to 1
  SNES_PORT &= ~(1<<SNES_LATCH); // latch is set to 0
  

  // Set up the shared port info for Saturn/Genesis
  SAT_DDR = 0x00; // initially set all Saturn/Genesis lines to input (this will be changed shortly)
  SAT_PORT = 0x00; // initially turn off all Saturn/Genesis pullup lines (this will be changed shortly)

  // Set up the Saturn/Gensis lines
  SAT_PORT |= (1<<SAT_D0) | (1<<SAT_D1) | (1<<SAT_D2) | (1<<SAT_D3); // turn on pullups for Saturn inputs  
  SAT_DDR  |= (1<<SAT_S0) | (1<<SAT_S1); // set S0 and S1 as outputs
  SAT_PORT &= ~( (1<<SAT_S0) | (1<<SAT_S1) ); // set S0 and S1 to 0

  // Set up the Genesis lines
  // turn on pullups for Gensis inputs (partially redundant, i.e. GEN_D0-3, as Saturn already set D0-3)
  GEN_PORT |= (1<<GEN_D0) | (1<<GEN_D1) | (1<<GEN_D2) | (1<<GEN_D3) | (1<<GEN_D4) | (1<<GEN_D5);   
  //GEN_DDR  |= (1<<GEN_SEL); // set Sel as output (redundant as Saturn already set S0 to output)
  //GEN_PORT &= ~(1<<GEN_SEL); // set Sel 0 (redundant as Saturn already set S0 to 0)

  // Clear the controller registers
  snesControllerRegister = 0x0000;
  satControllerRegister  = 0x0000;
  genControllerRegister  = 0x0000;

  delayMicroseconds(10); // allow pins to settle

  // USB Joystick setup
  // All axis are set to the neutral position
  DigiJoystick.setX((byte) 0x80);
  DigiJoystick.setY((byte) 0x80);      
  DigiJoystick.setXROT((byte) 0x80);
  DigiJoystick.setYROT((byte) 0x80);
  DigiJoystick.setZROT((byte) 0x80);
  DigiJoystick.setSLIDER((byte) 0x80);

  DigiJoystick.delay(15); // milliseconds
}




void loop() {
 
  // Read the state of the SNES controller (note, this is expected to take approximately 204us ... i.e. < 1ms)

  // Initalize the SNES controller by setting latch high for 12 uSeconds
  SNES_PORT |= (1<<SNES_LATCH); // set Latch to 1
  delayMicroseconds(12);
  SNES_PORT &= ~(1<<SNES_LATCH); // set Latch to 0

  // We first read the SNEScontroller button status into our 16 bit controller registers
  for(int i=0; i<16; i++) {
    delayMicroseconds(6);
    SNES_PORT &= ~(1<<SNES_CLOCK); // set Clock to 0
    
    // Read and store the button status for the SNES controller
    readStoreButtonState(snesControllerRegister, SNES_PIN, SNES_DATA, (0x01<<i),    true);

    delayMicroseconds(6);
    SNES_PORT |= (1<<SNES_CLOCK); // set Clock to 1
  }


  
  // Determine if we have a Genesis controller plugged in (i.e. D4 and D5 are 0 when Sel is 0)
  GEN_PORT &= ~(1<<GEN_SEL); // set Select to 0
  delayMicroseconds(10);

  uint8_t D4andD5 = GEN_PIN & ((1<<GEN_D4) | (1<<GEN_D5));
  if( D4andD5 == 0x00 ) {
    // Both D4 and D5 were 0 ("pressed") when Select was 0, so we treat this as a Genesis controller

    // Since we've switched to Genesis mode, we clear out the Saturn and Genesis controller registers
    satControllerRegister = 0x0000;
    genControllerRegister = 0x0000;
    
    // Read the values of D0-D3.  Store (interpreting as Genesis).

    readStoreButtonState(genControllerRegister, GEN_PIN, GEN_D0, GEN_UP_BUTTON_MASK,    true);
    readStoreButtonState(genControllerRegister, GEN_PIN, GEN_D1, GEN_DOWN_BUTTON_MASK,  true);
    readStoreButtonState(genControllerRegister, GEN_PIN, GEN_D2, GEN_A_BUTTON_MASK,     true);
    readStoreButtonState(genControllerRegister, GEN_PIN, GEN_D3, GEN_START_BUTTON_MASK, true);

    GEN_PORT |= (1<<GEN_SEL); // set Select to 1
    delayMicroseconds(10);

    // Read the values of D0-D5.  Store (interpreting as Genesis).
    // Note:  We don't need re-read D0 and D1 (Up/Down)

    readStoreButtonState(genControllerRegister, GEN_PIN, GEN_D2, GEN_B_BUTTON_MASK,     true);
    readStoreButtonState(genControllerRegister, GEN_PIN, GEN_D3, GEN_C_BUTTON_MASK,     true);
    readStoreButtonState(genControllerRegister, GEN_PIN, GEN_D4, GEN_LEFT_BUTTON_MASK,  true);
    readStoreButtonState(genControllerRegister, GEN_PIN, GEN_D5, GEN_RIGHT_BUTTON_MASK, true);

  } else {
    // Either D4 or D5 (or both) were 1, we treat this as a Saturn controller

    // Since we've switched to Saturn mode, we clear out the Saturn and Genesis controller registers
    satControllerRegister = 0x0000;
    genControllerRegister = 0x0000;

    // We have arrived in the state where [S1,S0] is [0,0]

    readStoreButtonState(satControllerRegister, SAT_PIN, SAT_D0, SAT_Z_BUTTON_MASK, true);
    readStoreButtonState(satControllerRegister, SAT_PIN, SAT_D1, SAT_Y_BUTTON_MASK, true);
    readStoreButtonState(satControllerRegister, SAT_PIN, SAT_D2, SAT_X_BUTTON_MASK, true);
    readStoreButtonState(satControllerRegister, SAT_PIN, SAT_D3, SAT_R_BUTTON_MASK, true);


    // Set S0/Sel to 1(5v).
    SAT_PORT |= (1<<SAT_S0); // set S0 to 1
    delayMicroseconds(10);

    // We have arrived in the state where [S1,S0] is [0,1]

    // Read the values of D0-D3.  Store (interpreting as Saturn).
    readStoreButtonState(satControllerRegister, SAT_PIN, SAT_D0, SAT_B_BUTTON_MASK,     true);
    readStoreButtonState(satControllerRegister, SAT_PIN, SAT_D1, SAT_C_BUTTON_MASK,     true);
    readStoreButtonState(satControllerRegister, SAT_PIN, SAT_D2, SAT_A_BUTTON_MASK,     true);
    readStoreButtonState(satControllerRegister, SAT_PIN, SAT_D3, SAT_START_BUTTON_MASK, true);


    // Set S1 to 1(5v).
    SAT_PORT |= (1<<SAT_S1); // set S1 to 1
    delayMicroseconds(10);

    // We have arrived in the state where [S1,S0] is [1,1]

    // Read the value of D3.  Store (interpreting as Saturn).
    readStoreButtonState(satControllerRegister, SAT_PIN, SAT_D3, SAT_L_BUTTON_MASK, true);


    // Set S0/Sel to 0(0v).
    SAT_PORT &= ~(1<<SAT_S0); // set S0 to 0
    delayMicroseconds(10);

    // We have arrived in the state where [S1,S0] is [1,0]

    // Read the values of D0-D3.  Store (interpreting as Saturn).
    readStoreButtonState(satControllerRegister, SAT_PIN, SAT_D0, SAT_UP_BUTTON_MASK,    true);
    readStoreButtonState(satControllerRegister, SAT_PIN, SAT_D1, SAT_DOWN_BUTTON_MASK,  true);
    readStoreButtonState(satControllerRegister, SAT_PIN, SAT_D2, SAT_LEFT_BUTTON_MASK,  true);
    readStoreButtonState(satControllerRegister, SAT_PIN, SAT_D3, SAT_RIGHT_BUTTON_MASK, true);


    // Set S1 to 0(0v).
    SAT_PORT &= ~(1<<SAT_S1); // set S1 to 0
    // We have arrived in the state where [S1,S0] is [0,0]

    //delayMicroseconds(10); 
  }


  // Buttons
  uint8_t snesButtons = (uint8_t)(snesControllerRegister & 0x000F) | (uint8_t)((snesControllerRegister & 0x0F00) >> 4);  // SNES buttons
  usbGameControllerRegisterLOW = snesButtons;

  // Special button combinations for the Saturn controller
  // The controller has no SELECT button, so SELECT = A+B+C.  Note that we'll block the individual Saturn A, B, and C, buttons while the SELECT combo is pressed.
  // This allows for SELECT and START to be pressed at the same time.
  // Also, during the time the SELECT combo is enabled, RIGHT, DOWN, UP, LEFT will map to SNES A, B, X, Y respectively.  To do this, we'll need to disable RIGHT, DOWN, UP, and LEFT
  
  if( (satControllerRegister & SAT_A_BUTTON_MASK) && (satControllerRegister & SAT_B_BUTTON_MASK) && (satControllerRegister & SAT_C_BUTTON_MASK) ) {
    
    // The SELECT combo was pressed.  Disable A, B, C, RIGHT, DOWN, UP, LEFT, and press SELECT
    uint8_t tempSatControllerRegister = satControllerRegister; // Used to build the new button states
    tempSatControllerRegister &= ~(SAT_A_BUTTON_MASK | SAT_B_BUTTON_MASK | SAT_C_BUTTON_MASK | SAT_RIGHT_BUTTON_MASK | SAT_DOWN_BUTTON_MASK | SAT_UP_BUTTON_MASK | SAT_LEFT_BUTTON_MASK);
    tempSatControllerRegister |= SNES_SELECT_BUTTON_MASK_POST_SHIFT;

    // Also, RIGHT, DOWN, UP, LEFT will map to SNES A, B, X, Y respectively.
    if(satControllerRegister & SAT_RIGHT_BUTTON_MASK) {
      tempSatControllerRegister |= SNES_A_BUTTON_MASK_POST_SHIFT;
      
    } else {
      tempSatControllerRegister &= ~(SNES_A_BUTTON_MASK_POST_SHIFT);
    }

    if(satControllerRegister & SAT_DOWN_BUTTON_MASK) {
      tempSatControllerRegister |= SNES_B_BUTTON_MASK_POST_SHIFT;
      
    } else {
      tempSatControllerRegister &= ~(SNES_B_BUTTON_MASK_POST_SHIFT);
    }

    if(satControllerRegister & SAT_UP_BUTTON_MASK) {
      tempSatControllerRegister |= SNES_X_BUTTON_MASK_POST_SHIFT;
      
    } else {
      tempSatControllerRegister &= ~(SNES_X_BUTTON_MASK_POST_SHIFT);
    }

    if(satControllerRegister & SAT_LEFT_BUTTON_MASK) {
      tempSatControllerRegister |= SNES_Y_BUTTON_MASK_POST_SHIFT;
      
    } else {
      tempSatControllerRegister &= ~(SNES_Y_BUTTON_MASK_POST_SHIFT);
    }

    satControllerRegister = tempSatControllerRegister; // now that we're finished building the new button status, we assign it
  }
  
  usbGameControllerRegisterLOW |= (uint8_t)(satControllerRegister & 0x00FF);  // we OR in the Saturn controller bits
  usbGameControllerRegisterHIGH = (uint8_t)( (satControllerRegister & (SAT_R_BUTTON_MASK | SAT_L_BUTTON_MASK)) >> 8 );  // Handle the Saturn controller's L and R buttons

  // Special button combinations for the Genesis 3-button controller
  // The controller has no SELECT button, so SELECT = A+B+C.  Note that we'll block the individual Genesis A, B, and C, buttons while the SELECT combo is pressed.
  // This allows for SELECT and START to be pressed at the same time.
  // Also, during the time the SELECT combo is enabled, RIGHT, DOWN, UP, LEFT will map to SNES A, B, X, Y respectively.  To do this, we'll need to disable RIGHT, DOWN, UP, and LEFT
  
  if( (genControllerRegister & GEN_A_BUTTON_MASK) && (genControllerRegister & GEN_B_BUTTON_MASK) && (genControllerRegister & GEN_C_BUTTON_MASK) ) {
    
    // The SELECT combo was pressed.  Disable A, B, C, RIGHT, DOWN, UP, LEFT, and press SELECT
    uint8_t tempGenControllerRegister = genControllerRegister; // Used to build the new button states
    tempGenControllerRegister &= ~(GEN_A_BUTTON_MASK | GEN_B_BUTTON_MASK | GEN_C_BUTTON_MASK | GEN_RIGHT_BUTTON_MASK | GEN_DOWN_BUTTON_MASK | GEN_UP_BUTTON_MASK | GEN_LEFT_BUTTON_MASK);
    tempGenControllerRegister |= SNES_SELECT_BUTTON_MASK_POST_SHIFT;

    // Also, RIGHT, DOWN, UP, LEFT will map to SNES A, B, X, Y respectively.
    if(genControllerRegister & GEN_RIGHT_BUTTON_MASK) {
      tempGenControllerRegister |= SNES_A_BUTTON_MASK_POST_SHIFT;
      
    } else {
      tempGenControllerRegister &= ~(SNES_A_BUTTON_MASK_POST_SHIFT);
    }

    if(genControllerRegister & GEN_DOWN_BUTTON_MASK) {
      tempGenControllerRegister |= SNES_B_BUTTON_MASK_POST_SHIFT;
      
    } else {
      tempGenControllerRegister &= ~(SNES_B_BUTTON_MASK_POST_SHIFT);
    }

    if(genControllerRegister & GEN_UP_BUTTON_MASK) {
      tempGenControllerRegister |= SNES_X_BUTTON_MASK_POST_SHIFT;
      
    } else {
      tempGenControllerRegister &= ~(SNES_X_BUTTON_MASK_POST_SHIFT);
    }

    if(genControllerRegister & GEN_LEFT_BUTTON_MASK) {
      tempGenControllerRegister |= SNES_Y_BUTTON_MASK_POST_SHIFT;
      
    } else {
      tempGenControllerRegister &= ~(SNES_Y_BUTTON_MASK_POST_SHIFT);
    }

    genControllerRegister = tempGenControllerRegister; // now that we're finished building the new button status, we assign it
  }
  
  usbGameControllerRegisterLOW |= (uint8_t)(genControllerRegister & 0x00FF);  // we OR in the Genesis controller bits

  DigiJoystick.setButtons((char) usbGameControllerRegisterLOW , (char) usbGameControllerRegisterHIGH);  // send the buttons to the HID driver


  // Now that we have the button status for controllers, we set the appropriate USB HID controller buttons and axis

  // controller x-axis
  if( (snesControllerRegister & SNES_LEFT_BUTTON_MASK) || (satControllerRegister & SAT_LEFT_BUTTON_MASK) || (genControllerRegister & GEN_LEFT_BUTTON_MASK) ) {
    // left was pressed
    DigiJoystick.setX((byte) 0x00);

  } else if( (snesControllerRegister & SNES_RIGHT_BUTTON_MASK) || (satControllerRegister & SAT_RIGHT_BUTTON_MASK) || (genControllerRegister & GEN_RIGHT_BUTTON_MASK) ) {
    // right was pressed 
    DigiJoystick.setX((byte) 0xFF);

  } else {
    // D-Pad is centered in the X direction 
    DigiJoystick.setX((byte) 0x80);
  }
  
  // controller y-axis
  if( (snesControllerRegister & SNES_DOWN_BUTTON_MASK) || (satControllerRegister & SAT_DOWN_BUTTON_MASK) || (genControllerRegister & GEN_DOWN_BUTTON_MASK) )  {
    // down was pressed
    DigiJoystick.setY((byte) 0xFF); // y-axis is flipped.  FF is down, 00 is up

  } else if( (snesControllerRegister & SNES_UP_BUTTON_MASK) || (satControllerRegister & SAT_UP_BUTTON_MASK) || (genControllerRegister & GEN_UP_BUTTON_MASK)) {
    // up was pressed 
    DigiJoystick.setY((byte) 0x00);

  } else {
    // D-Pad is centered in the Y direction 
    DigiJoystick.setY((byte) 0x80);
  }
  

  DigiJoystick.delay(15); // used to be 16. changed to 15 to compensate for extra instructions performed when translating controller registers to USB gamepad buttons/axis
}
