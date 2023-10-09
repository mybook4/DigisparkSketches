#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

//#include <Debug2Wire.h>

#define F_CPU 8000000UL // 8MHZ

//Debug2Wire gpioDebugger(14, 15);

// PSX pin definitions
#define PSX_PIN  PINB
#define PSX_PORT PORTB
#define PSX_DDR  DDRB
#define PSX_ATT  PB2
#define PSX_CMD  PB3
#define PSX_ACK  PB1
#define PSX_DAT  PB4
#define PSX_CLK  PB5

//          Bit0 Bit1 Bit2 Bit3 Bit4 Bit5 Bit6 Bit7
// byte0    SLCT           STRT UP   RGHT DOWN LEFT
// byte1    L2   R2    L1  R1   /\   O    X    |_|

// holds button data
volatile uint8_t buttonByte[2] = {0xFF, 0xFF};

// PSX Button Codes (correspond to masks for convenience)
#define PSX_UP       0x0010
#define PSX_DOWN     0x0040
#define PSX_LEFT     0x0080
#define PSX_RIGHT    0x0020
#define PSX_START    0x0008
#define PSX_SELECT   0x0001
#define PSX_X        0x4000
#define PSX_SQUARE   0x8000
#define PSX_O        0x2000
#define PSX_TRIANGLE 0x1000
#define PSX_R1       0x0800
#define PSX_R2       0x0200
#define PSX_L1       0x0400
#define PSX_L2       0x0100

// Saturn pin definitions
#define SAT_PIN  PIND
#define SAT_PORT PORTD
#define SAT_DDR  DDRD
#define SAT_S0 PD1
#define SAT_S1 PD0
#define SAT_D0 PD2
#define SAT_D1 PD3
#define SAT_D2 PD4
#define SAT_D3 PD5

// mapping between saturn buttons and PSX button bit positions
#define SAT_R_MAP  PSX_R2
#define SAT_X_MAP  PSX_L1
#define SAT_Y_MAP  PSX_TRIANGLE
#define SAT_Z_MAP  PSX_R1
#define SAT_St_MAP PSX_START
#define SAT_A_MAP  PSX_SQUARE
#define SAT_C_MAP  PSX_O
#define SAT_B_MAP  PSX_X
#define SAT_Rt_MAP PSX_RIGHT
#define SAT_Lt_MAP PSX_LEFT
#define SAT_Dn_MAP PSX_DOWN
#define SAT_Up_MAP PSX_UP
//#define SAT_L_MAP  PSX_SELECT
#define SAT_L_MAP PSX_L2

// Genesis pin definitions
#define GEN_PIN  PIND
#define GEN_PORT PORTD
#define GEN_DDR  DDRD
#define GEN_SEL PD0
#define GEN_D0 PD2
#define GEN_D1 PD3
#define GEN_D2 PD4
#define GEN_D3 PD5
#define GEN_D4 PD6
#define GEN_D5 PD7

// mapping between genesis buttons and PSX button bit positions
#define GEN_MODE_MAP PSX_SELECT
#define GEN_X_MAP  PSX_L1
#define GEN_Y_MAP  PSX_TRIANGLE
#define GEN_Z_MAP  PSX_R1
#define GEN_St_MAP PSX_START
#define GEN_A_MAP  PSX_SQUARE
#define GEN_C_MAP  PSX_O
#define GEN_B_MAP  PSX_X
#define GEN_Rt_MAP PSX_RIGHT
#define GEN_Lt_MAP PSX_LEFT
#define GEN_Dn_MAP PSX_DOWN
#define GEN_Up_MAP PSX_UP


// PSX protocol macros
// CMD
#define PSX_FRAME_START 0x01
#define PSX_BUTTON_POLL 0x42
#define PSX_CONFIG 0x43
#define PSX_CMD_BUTTON_QUERY_IDLE 0x00

// DAT
#define PSX_DIGITAL_ID 0x41
#define PSX_DATA_TO_FOLLOW 0x5A

// used to keep track of what byte in buttonByte[] we're going to send
volatile uint8_t current_byte = 0;

// used to trigger a delayed poll
volatile bool performDelayedPoll;

enum ProtocolState {
  INIT,
  REQ_TYPE,
  SEND_BUTTON_BYTE,
  HANDLE_CONFIG,
  FINISH
};

// stores the current state within the PSX protocol
ProtocolState current_state = INIT;

void setup() {
    // set up PSX lines
    PSX_DDR = 0x00;  // initially set all psx lines to input (this will be changed later)
    PSX_PORT = 0x00; // initially turn off all pullup lines
    PSX_PORT |= (1<<PSX_DAT); // Set the data line to high to start
    PSX_DDR |= (1<<PSX_DAT); // MISO is an output
  
    // set up SPI values for PSX lines
    SPCR = (1<<SPIE) | (1<<SPE) | (1<<DORD) | (1<<CPOL) | (1<<CPHA) | (1<<SPR1);
    
    SPDR = 0xFF;
    
    // set up Saturn controller lines
    SAT_DDR = 0x00;  // initially set all saturn lines to input (this will be changed later)
    SAT_PORT = 0x00; // initially turn off all pullup lines
    SAT_DDR |= (1<<SAT_S0) | (1<<SAT_S1); // set S0 and S1 to output (other lines are already inputs)
    

    // set up Genesis controller lines
    //GEN_DDR = 0x00;  // initially set all saturn lines to input (this will be changed later
    //GEN_PORT = 0x00; // initially turn off all pullup ines
    //GEN_DDR |= (1<<GEN_SEL);

    sei();
}


void setPSXButton(uint16_t button, bool pressButton) {
  if(pressButton) {
    buttonByte[1] &= ~(button>>8);
    buttonByte[0] &= ~(button); 
  } else {
    buttonByte[1] |= (button>>8);
    buttonByte[0] |= (button);
  }
}


void readSaturnButtonStates() {
  // set the buttonByte array with Saturn button states
  
  // set S1,S0 to 0,0, delay (to give controller time to settle), and read/store D0-D3
  SAT_PORT &= ~((1<<SAT_S1) | (1<<SAT_S0));
  _delay_us(10);
  (SAT_PIN & (1<<SAT_D0))? setPSXButton(SAT_Z_MAP, false): setPSXButton(SAT_Z_MAP, true); // if Z is 1, then it isn't pressed
  (SAT_PIN & (1<<SAT_D1))? setPSXButton(SAT_Y_MAP, false): setPSXButton(SAT_Y_MAP, true); // ...
  (SAT_PIN & (1<<SAT_D2))? setPSXButton(SAT_X_MAP, false): setPSXButton(SAT_X_MAP, true); // ...
  (SAT_PIN & (1<<SAT_D3))? setPSXButton(SAT_R_MAP, false): setPSXButton(SAT_R_MAP, true); // ...

  // set S1,S0 to 0,1
  SAT_PORT &= ~(1<<SAT_S1);
  SAT_PORT |= (1<<SAT_S0);
  _delay_us(10);
  (SAT_PIN & (1<<SAT_D0))? setPSXButton(SAT_B_MAP, false): setPSXButton(SAT_B_MAP, true); // if B is 1, then it isn't pressed
  (SAT_PIN & (1<<SAT_D1))? setPSXButton(SAT_C_MAP, false): setPSXButton(SAT_C_MAP, true); // ...
  (SAT_PIN & (1<<SAT_D2))? setPSXButton(SAT_A_MAP, false): setPSXButton(SAT_A_MAP, true); // ...
  (SAT_PIN & (1<<SAT_D3))? setPSXButton(SAT_St_MAP, false): setPSXButton(SAT_St_MAP, true); // ...

  // Holding A+B+C simulates pressing the PSX Select button
  ( ((SAT_PIN & (1<<SAT_D2)) == 0) && ((SAT_PIN & (1<<SAT_D0)) == 0) && ((SAT_PIN & (1<<SAT_D1)) == 0))? setPSXButton(PSX_SELECT, true): setPSXButton(PSX_SELECT, false);

  // set S1,S0 to 1,0
  SAT_PORT &= ~(1<<SAT_S0);
  SAT_PORT |= (1<<SAT_S1);
  _delay_us(10);
  (SAT_PIN & (1<<SAT_D0))? setPSXButton(SAT_Up_MAP, false): setPSXButton(SAT_Up_MAP, true); // if Up is 1, then it isn't pressed
  (SAT_PIN & (1<<SAT_D1))? setPSXButton(SAT_Dn_MAP, false): setPSXButton(SAT_Dn_MAP, true); // ...
  (SAT_PIN & (1<<SAT_D2))? setPSXButton(SAT_Lt_MAP, false): setPSXButton(SAT_Lt_MAP, true); // ...
  (SAT_PIN & (1<<SAT_D3))? setPSXButton(SAT_Rt_MAP, false): setPSXButton(SAT_Rt_MAP, true); // ...
  
  // set S1,S0 to 1,1
  SAT_PORT |= ((1<<SAT_S1) | (1<<SAT_S0));
  _delay_us(10);
  (SAT_PIN & (1<<SAT_D3))? setPSXButton(SAT_L_MAP, false): setPSXButton(SAT_L_MAP, true);
}



void readGenesisButtonStates() {
  // Note, currently 6 button controllers aren't yet supported
  // If needed, support can be added by clocking SEL 5 times (high to low) 
  //   and looking at the 3rd and 4th lows (6 button controllers have lo lo lo lo, then hi hi hi hi)

  // set the buttonByte array with Genesis button states

  // set SEL to 0, delay (to give controller time to settle), and read/store D0-D5
  GEN_PORT &= ~(1<<GEN_SEL);
  _delay_us(10);
  (GEN_PIN & (1<<GEN_D0))? setPSXButton(GEN_Up_MAP, false): setPSXButton(GEN_Up_MAP, true);
  (GEN_PIN & (1<<GEN_D1))? setPSXButton(GEN_Dn_MAP, false): setPSXButton(GEN_Dn_MAP, true);
  (GEN_PIN & (1<<GEN_D4))? setPSXButton(GEN_A_MAP, false): setPSXButton(GEN_A_MAP, true);
  (GEN_PIN & (1<<GEN_D5))? setPSXButton(GEN_St_MAP, false): setPSXButton(GEN_St_MAP, true);
  
  // Holding Start and A buttons simulates pressing the PSX Select Button
  ( ((GEN_PIN & (1<<GEN_D4)) == 0) && ((GEN_PIN & (1<<GEN_D5)) == 0) )? setPSXButton(PSX_SELECT, true): setPSXButton(PSX_SELECT, false);

  GEN_PORT |= (1<<GEN_SEL);
  _delay_us(10);
  (GEN_PIN & (1<<GEN_D2))? setPSXButton(GEN_Lt_MAP, false): setPSXButton(GEN_Lt_MAP, true);
  (GEN_PIN & (1<<GEN_D3))? setPSXButton(GEN_Rt_MAP, false): setPSXButton(GEN_Rt_MAP, true);
  (GEN_PIN & (1<<GEN_D4))? setPSXButton(GEN_B_MAP, false): setPSXButton(GEN_B_MAP, true);
  (GEN_PIN & (1<<GEN_D5))? setPSXButton(GEN_C_MAP, false): setPSXButton(GEN_C_MAP, true);
}


void send_ACK() {
  // pull ACK low for 3us, release (or set high instead?)
  
  _delay_us(5); // emulates 13us due to ancillary instructions (entering the function, etc)
  PSX_DDR |= (1<<PSX_ACK);  // pull ACK low
  
  _delay_us(3);
  PSX_DDR &= ~(1<<PSX_ACK);  // release ACK
}

void clearAndReturnToInit() {
  SPDR = 0xFF;
  current_state = INIT;
  current_byte = 0;
}

ISR(SPI_STC_vect) {
    
  uint8_t receivedByte = SPDR;
 
  switch(current_state) {
    
    case INIT:

      if(receivedByte == PSX_FRAME_START) {
        send_ACK();
        SPDR = PSX_DIGITAL_ID;
        current_state = REQ_TYPE;

      } else {
        clearAndReturnToInit();
      }

      break;

    case REQ_TYPE:
    
      performDelayedPoll = true;
      
      if(receivedByte == PSX_BUTTON_POLL) {
        send_ACK();
        SPDR = PSX_DATA_TO_FOLLOW;
        current_state = SEND_BUTTON_BYTE;

      } else if(receivedByte == PSX_CONFIG) {
        send_ACK();
        SPDR = PSX_DATA_TO_FOLLOW;
        current_state = HANDLE_CONFIG;

      } else {
        clearAndReturnToInit();
      }

      break;

    case SEND_BUTTON_BYTE:

      if(receivedByte == PSX_CMD_BUTTON_QUERY_IDLE) {
        send_ACK();
        SPDR = buttonByte[current_byte];
        current_byte++;

        if(current_byte == sizeof(buttonByte)) {
          current_state = FINISH;
        }

      } else {
        clearAndReturnToInit();
      }

      break;

    case HANDLE_CONFIG:

      if(receivedByte == 0x00) {
        send_ACK();
        SPDR = 0x01;
        current_state = FINISH;

      } else {
        clearAndReturnToInit();
      }

      break;

    case FINISH:
      clearAndReturnToInit();
      break;
  } 

}


void loop() {
  
  // Read the button states
  if(performDelayedPoll) {
    
    _delay_ms(15);
    readSaturnButtonStates(); // read the Sega Saturn controller button states
    //readGenesisButtonStates(); // read the Sega Genesis controller button states
    
    //gpioDebugger.debugPrintMSb(0x41);
    
    performDelayedPoll = false; // reset the flag
  }
}
