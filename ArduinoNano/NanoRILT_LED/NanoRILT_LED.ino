/////////////////////////////////////////////////////////////////////////////////////
// The Ridiculously Inexpensive Lag Tester (RILT) - Arduino Nano edition
//
// Note:  This is the 240p/480p Y (luma) version (Y of YPbPbr) for the Nano.
// The Arduino Nano (16Mhz) is used (which has an ATMega328p).
//
// Port B is used.
//
// PB0 - Negative Sync Active (0v), Negative Sync Inactive (5v)
// PB1 - Black (0v), White (5v)
//       PB0 and PB1 are used to generate the Y signal, which has built-in sync
// 
//       Sync
//       PB0-----/\/\/\/------|
//               1000 Ohm     |
//       B/W                  |
//       PB1-----/\/\/\/------*-----Output To Display (Vo)... ---/\/\/\/---GND
//               470 Ohm                                         75 Ohm (term)
//                      
//
//       PB1 | PB0 | ~Vo (terminated)
//       --------------------------
//       0v  |  0v | 0v (in sync pulse)
//       0v  |  5v | 0.304v (Black)
//       5v  |  0v | 0.646v (Grey)
//       5v  |  5v | 0.950v (White)
//
//       Vo (terminated) = ((Vpb1/Rpb1) + (Vpb0/Rpb0)) / ((1/75)+(1/Rpb0)+(1/Rpb10)
// 
//
// PB3 - Mode select (240p/480p)
//       Pull PB3 to ground for 480p mode.
//       Leave PB3 pulled up to 5v for 240p mode (ATMega328p internal pullup enabled).
// 
// 
// Concept: The goal was to create an inexpensive analog display lag testing device.
//          The design consists of a small microcontroller board (Arduino Nano),
//          a couple of resistors, and a single RCA connector for analog video output.
//          Since the ATMega328P has a 16-bit timer (Timer1), we use this.
//          We set up Timer1 for CTC mode, and set the OCR1A value appropriately.
//          A white box is drawn on the screen.  The Arduino Nano's onboard LED is lit 
//          when the first line of the white box is being drawn.
//          A reasonably high speed camera (e.g. 240fps smartphone camera) can be used
//          to count the difference in the number of frames between when the LED is lit,
//          and when the white box is displayed on the screen.
//
/////////////////////////////////////////////////////////////////////////////////////

#include<avr/sleep.h>

#define RILT_PORT PORTB
#define RILT_PIN  PINB
#define RILT_DDR  DDRB

#define SYNC_PORT_PIN 0    // PB0
#define VID_PORT_PIN  1    // PB1
#define VIDMODE_PORT_PIN 3 // PB3
#define LED_PIN 5          // PB5

#define BOX_START_LINE_240P 125
#define BOX_END_LINE_240P 146

#define BOX_START_LINE_480P 265
#define BOX_END_LINE_480P 305

#define VIDMODE_240P 0
#define VIDMODE_480P 1

#define RESULT_WAIT_IN_LINES 62492

#define OCR1A_VALUE_FOR_240P 1016 // 16,000,000 / (1015+1) = 15,748 Hz
#define OCR1A_VALUE_FOR_480P 508  // 16,000,000 / (507+1)  = 31,496 Hz

uint8_t vidMode;

uint16_t horizontalLineCount;
uint8_t frameCount;
bool drawBox;

void setup() {

  // Initialize GPIO
  
  // Set our Sync, Video, and LED pins to output mode
  RILT_DDR |= ( (1<<SYNC_PORT_PIN) | (1<<VID_PORT_PIN) | (1<<LED_PIN) );

  // Set our Mode and Sensor pins to input mode
  RILT_DDR &= ~(1<<VIDMODE_PORT_PIN); 

  // Set our Sync and Video pins to an initial value of sync inactive / black
  RILT_PORT &= ~(1<<VID_PORT_PIN);
  RILT_PORT |= (1<<SYNC_PORT_PIN);

  // Set the LED pin to 0 (LED off)
  RILT_PORT &= ~(1<<LED_PIN);

  // Activate pullup resistor for the Mode pins.
  RILT_PORT |= (1<<VIDMODE_PORT_PIN);
  
  // Initialize Timer1
  cli(); // stop interrupts
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1 = 0;  // counter is initially 0
  
  // Determine if we're in 240p or 480p mode.
  uint8_t mode = RILT_PIN & (1<<VIDMODE_PORT_PIN);
  if(mode) {
    // We're in 240p mode (the Mode pin was high)
    vidMode = VIDMODE_240P;
    OCR1A = OCR1A_VALUE_FOR_240P;
    
  } else {
    // We're in 480p mode (the Mode pin was low)
    vidMode = VIDMODE_480P;
    OCR1A = OCR1A_VALUE_FOR_480P;
  }

  // Continue initializing Timer1
  TCCR1B |= (1<<WGM12);  // Turn on CTC mode
  TCCR1B |= (1<<CS10);   // clk/1 prescaler
  TIMSK1 |= (1<<OCIE1A); // enable timer compare interrupt

  horizontalLineCount = 1;
  frameCount = 1;
  drawBox = false;

  set_sleep_mode(SLEEP_MODE_IDLE);

  sei();
}



////////////////////////////// Implementation which generates 240p //////////////////////////
void generate240p() {

  // Set to sync inactive / black
  // Vid should already be 0
  RILT_PORT &= ~(1<<VID_PORT_PIN);
  RILT_PORT |= (1<<SYNC_PORT_PIN);

  // We're in the Front Porch region
  // for 1.5us (approx 24 clock ticks at 16Mhz)
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t"); // 10th clock
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t"); // 20th clock
  __asm__ __volatile__ ("nop\n\t"); // 21st clock (overhead)

  // We enter the sync pulse for 4.7us (approximately 75 clock ticks at 16Mhz)
  // Set to sync active
  RILT_PORT &= ~(1<<SYNC_PORT_PIN);
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t"); // 10th clock
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t"); // 20th clock
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t"); // 30th clock
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t"); // 40th clock
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t"); // 50th clock
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t"); // 60th clock
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t"); // 64th clock (overhead)


  // If we're in the VSync region (i.e. on line 4, 5, or 6 (3 FP, 3 SP, 16 BP) for 240p,
  // then keep sync LOW (the back porch is 0 during VSync)
  // The Back Porch is 1.5us, or approximately 24 clock ticks at 16Mhz.

  // Note: The if() is structured this way to descrease jitter for 
  // when we're not in the VSync region.  The idea is that all parts
  // of the AND would need to be evaluated to true.  This might need to be 
  // replaced with assembly to prevent compiler optimizations from getting in
  // our way of our goal to reduce jitter.
  
  if( (horizontalLineCount != 4) && (horizontalLineCount != 5) && (horizontalLineCount != 6) ) {
    
    // We're not in the VSync region
    
    // Set to sync inactive / black
    RILT_PORT |= (1<<SYNC_PORT_PIN);
    
    if(horizontalLineCount == 262) {
      // We've reached the last line
      // The last things we do is reset the line counter and return
      horizontalLineCount = 1;
      frameCount++;
      return;
    }

    // If we're in the region to draw the line white,
    // then wait the rest of the Back Porch duration, 
    // then draw the line white
    if( (horizontalLineCount >= BOX_START_LINE_240P) && (horizontalLineCount < BOX_END_LINE_240P) ) {

      // Waiting for back porch to complete (approximately 25 clocks minus overhead)
      __asm__ __volatile__ ("nop\n\t");
      __asm__ __volatile__ ("nop\n\t");
      __asm__ __volatile__ ("nop\n\t");
      __asm__ __volatile__ ("nop\n\t");
      __asm__ __volatile__ ("nop\n\t");
      __asm__ __volatile__ ("nop\n\t");
      __asm__ __volatile__ ("nop\n\t");
      __asm__ __volatile__ ("nop\n\t");
      __asm__ __volatile__ ("nop\n\t"); // 10th clock

      // We draw the white box for approximately one second (60 frames),
      // out of 256 frames (approximately 4.1 seconds)
      if( (frameCount >= 60) && (frameCount < 120) ) {
        // set Vid to high (i.e. start drawing a white line of the box)
        RILT_PORT |= (1<<VID_PORT_PIN);

        if(horizontalLineCount == BOX_START_LINE_240P) {
          // This is the first white line of the box we're drawing
          // Light up the LED
          RILT_PORT |= (1<<LED_PIN);
        }

        // Wait for 25% of the screen to turn Vid back to black
        // The visible line time for 240p is 
        // 63.55us -1.5us(FP) -4.7us(Sync) -1.5us(BP) = 55.85us.
        // 0.25 * 55.85us = 13.96us
        // 13.96us / 0.0625 = 223 cycles
        // Doesn't have to be exact, just enough to make a visible white box.
        uint8_t waitCounter = 21;
        while(waitCounter) {
          __asm__ __volatile__ ("nop\n\t");
          __asm__ __volatile__ ("nop\n\t");
          __asm__ __volatile__ ("nop\n\t");
          __asm__ __volatile__ ("nop\n\t");
          __asm__ __volatile__ ("nop\n\t");
          __asm__ __volatile__ ("nop\n\t");
          __asm__ __volatile__ ("nop\n\t");
          __asm__ __volatile__ ("nop\n\t");
          __asm__ __volatile__ ("nop\n\t"); // 10th clock
          waitCounter--;
        }

        RILT_PORT &= ~(1<<VID_PORT_PIN);
        
      } else {
        // We're not in the draw period, simply turn off the LED
        RILT_PORT &= ~(1<<LED_PIN);
      }
      
    } else if(horizontalLineCount == 50) {
      // draw a pilot line
      // Waiting for back porch to complete (approximately 25 clocks minus overhead)
      __asm__ __volatile__ ("nop\n\t");
      __asm__ __volatile__ ("nop\n\t");
      __asm__ __volatile__ ("nop\n\t");
      __asm__ __volatile__ ("nop\n\t");
      __asm__ __volatile__ ("nop\n\t");
      __asm__ __volatile__ ("nop\n\t");
      __asm__ __volatile__ ("nop\n\t");
      __asm__ __volatile__ ("nop\n\t");
      __asm__ __volatile__ ("nop\n\t"); // 10th clock

      __asm__ __volatile__ ("nop\n\t");
      __asm__ __volatile__ ("nop\n\t");
      __asm__ __volatile__ ("nop\n\t");
      __asm__ __volatile__ ("nop\n\t");
      __asm__ __volatile__ ("nop\n\t");
      __asm__ __volatile__ ("nop\n\t");
      __asm__ __volatile__ ("nop\n\t");
      __asm__ __volatile__ ("nop\n\t");
      __asm__ __volatile__ ("nop\n\t"); // 20th clock

      // set Vid to high (i.e. start drawing a white line of the box)
        RILT_PORT |= (1<<VID_PORT_PIN);
    }
    
  } else {
    // We're in the VSync region.
    // Keep sync line LOW.
    
  }

  horizontalLineCount++;
  return;
}





////////////////////////////// Implementation which generates 480p //////////////////////////
void generate480p() {

  // Set to sync inactive / black
  // Vid should already be 0
  RILT_PORT &= ~(1<<VID_PORT_PIN);
  RILT_PORT |= (1<<SYNC_PORT_PIN);

  // We're in the Front Porch region
  // for 0.5925us, however, we need to maintain good VSync pulse timing
  // so we choose 2.33us (approx 37 clock ticks at 16Mhz)
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t"); // 10th clock
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t"); // 20th clock
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t"); // 30th clock
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t"); // 32nd clock (overhead)

  // We enter the sync pulse for 2.33s (approximately 37 clock ticks at 16Mhz)
  // Set to sync active
  RILT_PORT &= ~(1<<SYNC_PORT_PIN);
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t"); // 10th clock
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t"); // 20th clock
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t"); // 25th clock (overhead)


  // If we're in the VSync region (i.e. on line 7, 8, 9, 10, 11, or 12, (6 FP, 6 SP, 32 BP) for 480p,
  // then keep sync LOW (the back porch is 0 during VSync)
  // The Back Porch is 2.1886us, or approximately 35 clock ticks at 16Mhz.

  // Note: The if() is structured this way to descrease jitter for 
  // when we're not in the VSync region.  The idea is that all parts
  // of the AND would need to be evaluated to true.  This might need to be 
  // replaced with assembly to prevent compiler optimizations from getting in
  // our way of our goal to reduce jitter.

  if( (horizontalLineCount > 6) && (horizontalLineCount < 13) ) {
    // We're in the VSync region.
    // Keep sync line LOW.
    
  } else {
    // We're not in the VSync region
    
    // Set to sync inactive / black
    RILT_PORT |= (1<<SYNC_PORT_PIN);
    
    if(horizontalLineCount == 525) {
      // We've reached the last line
      // The last things we do is reset the line counter and return
      horizontalLineCount = 1;
      frameCount++;
      return;
    }

    // If we're in the region to draw the line white,
    // then wait the rest of the Back Porch duration, 
    // then draw the line white
    if( (horizontalLineCount >= BOX_START_LINE_480P) && (horizontalLineCount < BOX_END_LINE_480P) ) {

      // Waiting for back porch to complete (approximately 31 clocks minus overhead)
      __asm__ __volatile__ ("nop\n\t");
      __asm__ __volatile__ ("nop\n\t");
      __asm__ __volatile__ ("nop\n\t");
      __asm__ __volatile__ ("nop\n\t");
      __asm__ __volatile__ ("nop\n\t");
      __asm__ __volatile__ ("nop\n\t");
      __asm__ __volatile__ ("nop\n\t");
      __asm__ __volatile__ ("nop\n\t");
      __asm__ __volatile__ ("nop\n\t"); // 10th clock
      __asm__ __volatile__ ("nop\n\t");
      __asm__ __volatile__ ("nop\n\t");
      __asm__ __volatile__ ("nop\n\t");
      __asm__ __volatile__ ("nop\n\t"); // 14th clock

      // We draw the white box for approximately one second (60 frames),
      // out of 256 frames (approximately 4.1 seconds)
      if( (frameCount >= 60) && (frameCount < 120) ) {
        // set Vid to high (i.e. start drawing a white line of the box)
        RILT_PORT |= (1<<VID_PORT_PIN);

        if(horizontalLineCount == BOX_START_LINE_480P) {
          // This is the first white line of the box we're drawing
          // Light up the LED
          RILT_PORT |= (1<<LED_PIN);
        }

        // Wait for 25% of the screen to turn Vid back to black
        // Doesn't have to be exact, just enough to make a visible white box.
        uint8_t waitCounter = 9;
        while(waitCounter) {
          __asm__ __volatile__ ("nop\n\t");
          __asm__ __volatile__ ("nop\n\t");
          __asm__ __volatile__ ("nop\n\t");
          __asm__ __volatile__ ("nop\n\t");
          __asm__ __volatile__ ("nop\n\t");
          __asm__ __volatile__ ("nop\n\t");
          __asm__ __volatile__ ("nop\n\t");
          __asm__ __volatile__ ("nop\n\t");
          __asm__ __volatile__ ("nop\n\t"); // 10th clock
          waitCounter--;
        }

        RILT_PORT &= ~(1<<VID_PORT_PIN);
        
      } else {
        // We're not in the draw period, simply turn off the LED
        RILT_PORT &= ~(1<<LED_PIN);
      }
      
    } else if(horizontalLineCount == 75) {
      // draw a pilot line
      // Waiting for back porch to complete
      __asm__ __volatile__ ("nop\n\t");
      __asm__ __volatile__ ("nop\n\t");
      __asm__ __volatile__ ("nop\n\t");
      __asm__ __volatile__ ("nop\n\t");
      __asm__ __volatile__ ("nop\n\t");
      __asm__ __volatile__ ("nop\n\t");
      __asm__ __volatile__ ("nop\n\t");
      __asm__ __volatile__ ("nop\n\t");
      __asm__ __volatile__ ("nop\n\t"); // 10th clock

      __asm__ __volatile__ ("nop\n\t");
      __asm__ __volatile__ ("nop\n\t");
      __asm__ __volatile__ ("nop\n\t");
      __asm__ __volatile__ ("nop\n\t");
      __asm__ __volatile__ ("nop\n\t");
      __asm__ __volatile__ ("nop\n\t");
      __asm__ __volatile__ ("nop\n\t");
      __asm__ __volatile__ ("nop\n\t");
      __asm__ __volatile__ ("nop\n\t"); // 20th clock

      __asm__ __volatile__ ("nop\n\t");
      __asm__ __volatile__ ("nop\n\t");
      __asm__ __volatile__ ("nop\n\t"); // 23rd clock

      // set Vid to high (i.e. start drawing a white line of the box)
        RILT_PORT |= (1<<VID_PORT_PIN);
    }
    
  }

  horizontalLineCount++;
  return;
}


////////////////////////////// Interrupt service routing for Timer1 compare //////////////////////////
ISR(TIMER1_COMPA_vect) { 

  if(vidMode == VIDMODE_240P) {
    generate240p();
  } else {
    generate480p();
  }

}

void loop() {
  sleep_mode();
}

