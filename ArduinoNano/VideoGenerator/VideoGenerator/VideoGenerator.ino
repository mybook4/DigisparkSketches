
// Note:  This is the 480p RGBS version.
// PORTB pin 0 is used for the Composite Sync line
// PORTC is also used for the R (PC0), G (PC1), B (PC2) lines

// Note:  This code is quick and dirty.  It is 
// subject to accuracy swings based on the
// clock accuracy of the Arduino Nano board used.

#include<avr/sleep.h>

#define OUTPUT_SYNC_DDR DDRB
#define OUTPUT_SYNC_PORT PORTB
#define OUTPUT_SYNC_PIN 0
#define OUTPUT_SYNC_HIGH 0x01
#define OUTPUT_SYNC_LOW 0x00

#define OUTPUT_RGB_DDR DDRC
#define OUTPUT_RGB_PORT PORTC
#define OUTPUT_R_PIN 0
#define OUTPUT_G_PIN 1
#define OUTPUT_B_PIN 2
#define OUTPUT_RGB_HIGH 0x07
#define OUTPUT_RGB_LOW 0x00


uint16_t horizontalLineCount;
uint16_t lagLineCounter;
bool measuringLag;

void setup()
{
  OUTPUT_SYNC_DDR = _BV(OUTPUT_SYNC_PIN);  // The sync pin is set to an output
  OUTPUT_SYNC_PORT = _BV(OUTPUT_SYNC_PIN); // The sync pin is set HIGH (5V)
  
  OUTPUT_RGB_DDR = _BV(OUTPUT_R_PIN) | _BV(OUTPUT_G_PIN) | _BV(OUTPUT_B_PIN);
  OUTPUT_RGB_PORT = OUTPUT_RGB_HIGH;
  
  // initialize timer1 
  TCCR1A = 0;
  TCCR1B = 0;
  TCCR1B |= (1 << CS10);    // clk/1 prescaler 
  TCNT1 = 65060;            // preload timer 508 cycles:  16,000,000 / (525*59.54) ~= 31.7777us
  TIMSK1 |= (1 << TOIE1);   // enable timer overflow interrupt

  horizontalLineCount = 1;
  measuringLag = false;
  lagLineCounter = 0;

  set_sleep_mode(SLEEP_MODE_IDLE);
  
  sei();
}


ISR(TIMER1_OVF_vect)        // interrupt service routine on Timer1 overflow
{
  TCNT1 = 65060;            // preload timer
  OUTPUT_SYNC_PORT = OUTPUT_SYNC_HIGH; // set the sync line to HIGH
  
  // set R, G, and B outputs to LOW
  OUTPUT_RGB_PORT = OUTPUT_RGB_LOW;

  // We're in the Front Porch for 16 pixels (0.635555us), or approximately 10 clock cycles at 16Mhz
  // Don't need to touch the sync line because it is already HIGH
  __asm__ __volatile__ ("nop\n\t");  
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t"); 
  __asm__ __volatile__ ("nop\n\t");   
  __asm__ __volatile__ ("nop\n\t"); // 9 cycles left because of overhead

  // We enter the sync pulse for 96 pixels (3.8133333us), or approximately 61 clock cycles at 16Mhz
  //OUTPUT_PORT &= ~(1<<OUTPUT_SYNC_PIN);  // set the sync line to LOW
  OUTPUT_SYNC_PORT = OUTPUT_SYNC_LOW; // set the sync line to LOW
  
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
  __asm__ __volatile__ ("nop\n\t");  // 50th clock (to compensate for the lost time below

  // If we're in the VSync region (i.e. on line 11 or 12 (10 FP, 2 SP, 33 BP) for 480p,
  // then keep CSync LOW (the back porch is 0 during VSync)
  // The Back Porch is 48 pixels (1.9066666us), or approximately 31 clocks (30.5, but round up to be safe)

  // Note: The IF is structured this way to descrease jitter for 
  // when we're not in the VSync region.  The idea is that both parts
  // of the AND would need to be evaluated to true.  This might need to be 
  // replaced with assembly to prevent compiler optimizations from getting in
  // our way of our goal to reduce jitter.
  
  if( (horizontalLineCount != 11) && (horizontalLineCount != 12) ) {
    
    // we're not in the VSync region
    OUTPUT_SYNC_PORT = OUTPUT_SYNC_HIGH; // set the sync line back to HIGH
    
    // If we're measuring input lag, then increment the lagLineCounter and return
    //if(measuringLag) {
    //  lagLineCounter++;
    //  return;
    //}
    
    if(horizontalLineCount == 525) {
      // We've reached the last line
      // The last thing we do is reset the line counter and return
      horizontalLineCount = 1;
      return;
    }

    // If we're in the region to draw the line white,
    // then wait the rest of the Back Porch duration, 
    // then draw the line white for approximately 25% of the line.
    if( (horizontalLineCount > 276) && (horizontalLineCount < 294) ) {

      // Waiting for back porch to complete (approximately 31 clocks minus above overhead)
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
      __asm__ __volatile__ ("nop\n\t"); // 14th clock

      // set R, G, and B outputs to HIGH
      OUTPUT_RGB_PORT = OUTPUT_RGB_HIGH;

      // wait for approximately 25% of the active line time 
      // (25.422222 / 4 = 6.3555us, or approximately 101 clocks
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
      __asm__ __volatile__ ("nop\n\t");  // 50th clock

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
      __asm__ __volatile__ ("nop\n\t");
      __asm__ __volatile__ ("nop\n\t");
      __asm__ __volatile__ ("nop\n\t");
      __asm__ __volatile__ ("nop\n\t");
      __asm__ __volatile__ ("nop\n\t");
      __asm__ __volatile__ ("nop\n\t");
      __asm__ __volatile__ ("nop\n\t"); // 70th clock

      __asm__ __volatile__ ("nop\n\t");
      __asm__ __volatile__ ("nop\n\t");
      __asm__ __volatile__ ("nop\n\t");
      __asm__ __volatile__ ("nop\n\t");
      __asm__ __volatile__ ("nop\n\t");
      __asm__ __volatile__ ("nop\n\t");
      __asm__ __volatile__ ("nop\n\t");
      __asm__ __volatile__ ("nop\n\t");
      __asm__ __volatile__ ("nop\n\t");
      __asm__ __volatile__ ("nop\n\t"); // 80th clock

      __asm__ __volatile__ ("nop\n\t");
      __asm__ __volatile__ ("nop\n\t");
      __asm__ __volatile__ ("nop\n\t");
      __asm__ __volatile__ ("nop\n\t");
      __asm__ __volatile__ ("nop\n\t");
      __asm__ __volatile__ ("nop\n\t");
      __asm__ __volatile__ ("nop\n\t");
      __asm__ __volatile__ ("nop\n\t");
      __asm__ __volatile__ ("nop\n\t");
      __asm__ __volatile__ ("nop\n\t"); // 90th clock

      __asm__ __volatile__ ("nop\n\t");
      __asm__ __volatile__ ("nop\n\t");
      __asm__ __volatile__ ("nop\n\t");
      __asm__ __volatile__ ("nop\n\t");
      __asm__ __volatile__ ("nop\n\t");
      __asm__ __volatile__ ("nop\n\t");
      __asm__ __volatile__ ("nop\n\t");
      __asm__ __volatile__ ("nop\n\t");
      __asm__ __volatile__ ("nop\n\t");
      __asm__ __volatile__ ("nop\n\t"); // 100th clock

      // set R, G, and B outputs to LOW
      OUTPUT_RGB_PORT = OUTPUT_RGB_LOW;
    } // the else (increment horizontalLineCount) is handled below
    
  } else {
    // we're in the VSync region
    // keep sync line LOW
    
    //if(measuringLag) {
    //  lagLineCounter++;
    //}

  }

  // The last thing we do before returning is increment the line count
  horizontalLineCount++;
  return;
}


void loop() {
  sleep_mode();
}

