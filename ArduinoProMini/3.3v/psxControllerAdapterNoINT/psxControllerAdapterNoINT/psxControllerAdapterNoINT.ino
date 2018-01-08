#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include <Debug2Wire.h>

#define F_CPU 8000000UL // 8MHZ

Debug2Wire gpioDebugger(14,15);

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

/*
#define PSX_UP       0x0008
#define PSX_DOWN     0x0002
#define PSX_LEFT     0x0001
#define PSX_RIGHT    0x0004
#define PSX_START    0x0010
#define PSX_SELECT   0x0080
#define PSX_X        0x0200
#define PSX_SQUARE   0x0100
#define PSX_O        0x0400
#define PSX_TRIANGLE 0x0800
#define PSX_R1       0x1000
#define PSX_R2       0x4000
#define PSX_L1       0x2000
#define PSX_L2       0x8000*/

// Saturn pin definitions
#define SAT_PIN  PIND
#define SAT_PORT PORTD
#define SAT_DDR  DDRD
#define SAT_S0 PD0 // AVR, not arduino number
#define SAT_S1 PD1
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
#define SAT_L_MAP  PSX_SELECT

// PSX protocol macros
// CMD
#define PSX_FRAME_START 0x01
//#define PSX_FRAME_START 0x80

#define PSX_BUTTON_POLL 0x42
//#define PSX_BUTTON_POLL 0x42

#define PSX_CONFIG 0x43
//#define PSX_CONFIG 0xC2

#define PSX_CMD_BUTTON_QUERY_IDLE 0x00
//#define PSX_CMD_BUTTON_QUERY_IDLE 0x00


// DAT
#define PSX_DIGITAL_ID 0x41
//#define PSX_DIGITAL_ID 0x82

#define PSX_DATA_TO_FOLLOW 0x5A
//#define PSX_DATA_TO_FOLLOW 0x5A


// used to keep track of what byte in buttonByte[] we're going to send
volatile uint8_t current_byte = 0;

// keeps stores the time when the interrupt routine last finished
volatile unsigned long time;

enum ProtocolState {
  INIT,
  REQ_TYPE,
  SEND_BUTTON_BYTE,
  HANDLE_CONFIG,
  FINISH
};

// stores the current state within the PSX protocol
ProtocolState current_state = INIT;

int numPolls;
bool pressUp;

void setup() {
    // set up PSX lines
    PSX_DDR = 0x00;  // initially set all psx lines to input (this will be changed later)
    PSX_PORT = 0x00; // initially turn off all pullup lines
    PSX_PORT |= (1<<PSX_DAT); // Set the data line to high to start
    PSX_DDR |= (1<<PSX_DAT); // MISO is an output
  
    // set up SPI values for PSX lines
    //PRR &= ~(1<<PRSPI); // Set to 0 to ensure power to SPI module
    //SPCR = 0x6e; //0b01101110;
    
    SPCR = (1<<SPE) | (1<<DORD) | (1<<CPOL) | (1<<CPHA) | (1<<SPR1);
    
    /*
    SPCR &= ~(1<<SPIE); // interrupt disabled
    SPCR |= (1<<DORD);  // Bytes are transmitted LSB first, MSB last
    SPCR &= ~(1<<MSTR); // we are an SPI slave
    SPCR |= (1<<CPOL);  
    SPCR |= (1<<CPHA);  
    */
    
    SPSR = (1<<SPI2X); // SPSR
    
    /*
    SPCR |= (1<<SPR1);
    SPCR &= ~(1<<SPR0);
    
    SPCR |= (1<<SPE);   // Enable SPI
    */
    
    SPDR = 0xFF;
    
    // set up Saturn controller lines
    SAT_DDR = 0x00;  // initially set all saturn lines to input (this will be changed later)
    SAT_PORT = 0x00; // initially turn off all pullup lines
    SAT_DDR |= (1<<SAT_S0) | (1<<SAT_S1); // set S0 and S1 to output (other lines are already inputs)
    
    // DEBUG  
    numPolls = 0;  
    pressUp = false;
    // END DEBUG
    
    //sei();
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

  // set S1,S0 to 1,0
  SAT_PORT |= (1<<SAT_S1);
  SAT_PORT &= ~(1<<SAT_S0);
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

uint8_t spi_ReceiveByte() {
  while( !(SPSR & (1<<SPIF)) );
  return(SPDR); 
}

void send_ACK() {
  // pull ACK low for 3us, release (or set high instead?)
  
  _delay_us(7);    
  PSX_DDR |= (1<<PSX_ACK);  // pull ACK low
  
  _delay_us(3);
  PSX_DDR &= ~(1<<PSX_ACK);  // release ACK
}

void clearAndReturnToInit() {
  SPDR = 0xFF;
  current_state = INIT;
  current_byte = 0;
}



void loop() {
  // Read the Saturn button states 14ms after the last interrupt
  /*if( (millis() - time) > 14) {
    readSaturnButtonStates();
  }*/
  /*
  setPSXButton(PSX_UP, true);
  _delay_ms(1000);
  setPSXButton(PSX_UP, false);
  _delay_ms(100);
  setPSXButton(PSX_DOWN, true);
  _delay_ms(1000);
  setPSXButton(PSX_DOWN, false);
  _delay_ms(1000);*/
  
  
  
  uint8_t receivedByte = spi_ReceiveByte(); // wait for a byte
  //gpioDebugger.debugPrintMSb(receivedByte); 
  
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

      if(receivedByte == PSX_BUTTON_POLL) {
        send_ACK();
        SPDR = PSX_DATA_TO_FOLLOW;
        current_state = SEND_BUTTON_BYTE;
        
        // DEBUG
        ++numPolls;
 
        if(numPolls >= 60) {
          
          if(pressUp) {
            setPSXButton(PSX_DOWN, false);
            setPSXButton(PSX_UP, true);
          } else {
            setPSXButton(PSX_UP, false);
            setPSXButton(PSX_DOWN, true);
          }
          
          pressUp = !pressUp;  // toggle state
          numPolls = 0;
        } // END DEBUG

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
        SPDR = 0x01; // 0x80;
        current_state = FINISH;

      } else {
        clearAndReturnToInit();
      }

      break;

    case FINISH:
      clearAndReturnToInit();
      break;
  } 
  
  //time = millis();
  
}
