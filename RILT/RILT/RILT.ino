/////////////////////////////////////////////////////////////////////////////////////
// The Ridiculously Inexpensive Lag Tester (RILT)
//
// Note:  This is the 240p/480p Y (luma) version (Y of YPbPbr) for the Digispark.
// The Digispark (small ATTiny85 board with Micronucleus installed)
// is used to leverage the somewhat reasonably accurate 16.5Mhz 
// derrived clock from USB connection.
//
// Port B is used (only port available on ATTiny85).
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
// PB2 - Connection to light sensor used to detect presence of white on the display.
//       PB2 requires a fast photodiode (e.g. recommend quicker than 100us response)
//       I am planning to use a Vishay TEFD4300 (100ns rise/fall time).
//
//       Vcc-----/\/\/\/----*-----|>|------GND
//               5k Ohm     |
//                         PB2   
//
// PB3 - Unused (after micronucleus hands off to code)
// PB4 - Unused (after micronucleus hands off to code)
// PB5 - Mode select (240p/480p)
//       Pull PB5 to ground for 480p mode. (NOT IMPLEMENTED YET)
//       Leave PB5 pulled up to 5v for 240p mode (ATTiny85 internal pullup enabled).
//       As a future enhancement, the anlog mode of this pin might be used to enable 
//       other values (e.g. 0<=X<1v for 480p, 2v<X<3v for 480i, X>=4 for 240p).
// 
// 
// Concept: The goal was to create an inexpensive analog display lag testing device.
//          The design consists of a small microcontroller board (Digispark),
//          a relatively fast photodiode (for display light detection), 
//          a couple of resistors, and a single RCA connector for analog video output.
//          
//
/////////////////////////////////////////////////////////////////////////////////////



#define RILT_PORT PORTB
#define RILT_PIN  PINB
#define RILT_DDR  DDRB

#define SYNC_PORT_PIN 0    // PB0
#define VID_PORT_PIN  1    // PB1
#define SENSOR_PORT_PIN 2  // PB2
#define VIDMODE_PORT_PIN 5 // PB5

#define BOX_START_LINE 125
#define BOX_END_LINE 146

// Result is expressed by {RESULT1,RESULT2,RESULT3,RESULT4},
// where the result is expressed by a 4-place-value octal number.
// This allows for shift operations (rather than expensive
// multiplication/division).  This also means that the maximum
// lag that can be measured is a bit over 1/4 of a second.
// Max 4-place-value octal number is decimal 4095 (7777).
// Since we're counting lines, 4095*63.55us = approx 0.26 seconds.
// NOTE:  In order to make place values easy to see, 
//        a zero value on each line is 1 tac mark, and a value
//        of seven is 8 tac marks.
// Example:  1000 lines in octal is 1750
//           The line display would show:
//           --          (1)
//           --------    (7)
//           ------      (5)
//           -           (0)
//
#define RESULT1_START_LINE BOX_START_LINE
#define RESULT2_START_LINE 132
#define RESULT3_START_LINE 139
#define RESULT4_START_LINE 145

#define VIDMODE_240P 0
#define VIDMODE_480I 1
#define VIDMODE_480P 2

uint8_t vidMode;
uint8_t timerCycles;

uint16_t horizontalLineCount;

bool measuringLag;
bool drawBox;
uint16_t lagLineCounter;
uint8_t resultFramesToShow;


void setup() {
  // Set our Sync and Video pins to output mode
  RILT_DDR |= (1<<SYNC_PORT_PIN);
  RILT_DDR |= (1<<VID_PORT_PIN);

  // Set our Mode and Sensor pins to input mode
  RILT_DDR &= ~(1<<VIDMODE_PORT_PIN); 
  RILT_DDR &= ~(1<<SENSOR_PORT_PIN);

  // Set our Sync and Video pins to an initial value of sync inactive / black
  RILT_PORT &= ~(1<<VID_PORT_PIN);
  RILT_PORT |= (1<<SYNC_PORT_PIN);

  // Activate pullup resistors for the Mode and Sensor pins.
  RILT_PORT |= (1<<VIDMODE_PORT_PIN);
  RILT_PORT |= (1<<SENSOR_PORT_PIN);

  // Determine if we're in 240p or 480p mode.  
  if(RILT_PIN & (1<<VIDMODE_PORT_PIN)) {
    // We're in 240p mode (the Mode pin was high)
    vidMode = VIDMODE_240P;
  } // 480p and 480i modes not implemented yet

  // initialize timer0 
  TCCR0A = 0;
  TCCR0B = 0;
  TCCR0B |= (1 << CS00);       // clk/1 prescaler
  TCNT0 = 0;                   // preload timer
  TIMSK |= (1 << TOIE0);       // enable timer overflow interrupt

  horizontalLineCount = 1;
  timerCycles = 0;
  drawBox = true;
  measuringLag = true;
  lagLineCounter = 0;
  resultFramesToShow = 255;
  
  sei();
}


ISR(TIMER0_OVF_vect)        // interrupt service routine on Timer0 overflow
{
  //TCNT0 = TIMER_PRELOAD_VALUE;  // preload the timer

  // Assume 240p for now (480p and 480i not yet supported)

  // We're limited by an 8-bit counter, so we need to ensure there have been 3 counter cycles before continuing
  if(timerCycles) {
    // The timer cycle counter is not zero.

    if(timerCycles == 3) {
      // We've gone through 3 complete timer cycles (256x3 = 768 clocks = approx 46.55us).
      // We reset the timer counter
      timerCycles = 0;

      // We set TCNT0 to so we can land on 63.55 at the beginning of timer cycle 0
      TCNT0 = 13;
      
      // The last thing we do before returning is increment the line count
      horizontalLineCount++;
      
    } else if(timerCycles == 2) {
      // We've gone through 2 timer cycles

      // Set Vid to low, since we want to show black here after showing white for the first 2 timer cycles
      RILT_PORT &= ~(1<<VID_PORT_PIN);

      timerCycles++;

      // If we're in the box, we draw the results if we have them

      // If measuringLag was found to be false, then it means light was detected.
      // lagLineCounter should contain the number of lines counted before the light was detected.
      if(!measuringLag) {

        if( horizontalLineCount == RESULT1_START_LINE ) { 

          // The result1 value is bits 11, 10, 9 (0-8 cut off)
          uint8_t result1 = (lagLineCounter >> 9) & 0x07;
          result1++;
          
          //result1 = 7;
          while(result1) {
            __asm__ __volatile__ ("nop\n\t");
            __asm__ __volatile__ ("nop\n\t");
            __asm__ __volatile__ ("nop\n\t");
            __asm__ __volatile__ ("nop\n\t");
            __asm__ __volatile__ ("nop\n\t");
            __asm__ __volatile__ ("nop\n\t");
            __asm__ __volatile__ ("nop\n\t");
            __asm__ __volatile__ ("nop\n\t");
            RILT_PORT |= (1<<VID_PORT_PIN); // Set to white
            __asm__ __volatile__ ("nop\n\t");
            __asm__ __volatile__ ("nop\n\t");
            __asm__ __volatile__ ("nop\n\t");
            __asm__ __volatile__ ("nop\n\t");
            __asm__ __volatile__ ("nop\n\t");
            __asm__ __volatile__ ("nop\n\t");
            RILT_PORT &= ~(1<<VID_PORT_PIN); // Set to black
            result1--;
          }
        } else if( horizontalLineCount == RESULT2_START_LINE ) {

          // The result2 value is bits 8, 7, 6 (0-5 cut off)
          uint8_t result2 = (lagLineCounter >> 6) & 0x07;
          result2++;
          
          //result2 = 7;
          while(result2) {
            __asm__ __volatile__ ("nop\n\t");
            __asm__ __volatile__ ("nop\n\t");
            __asm__ __volatile__ ("nop\n\t");
            __asm__ __volatile__ ("nop\n\t");
            __asm__ __volatile__ ("nop\n\t");
            __asm__ __volatile__ ("nop\n\t");
            __asm__ __volatile__ ("nop\n\t");
            __asm__ __volatile__ ("nop\n\t");
            RILT_PORT |= (1<<VID_PORT_PIN); // Set to white
            __asm__ __volatile__ ("nop\n\t");
            __asm__ __volatile__ ("nop\n\t");
            __asm__ __volatile__ ("nop\n\t");
            __asm__ __volatile__ ("nop\n\t");
            __asm__ __volatile__ ("nop\n\t");
            __asm__ __volatile__ ("nop\n\t");
            RILT_PORT &= ~(1<<VID_PORT_PIN); // Set to black
            result2--;
          }
        } else if( horizontalLineCount == RESULT3_START_LINE ) {

          // The result3 value is bits 5, 4, 3 (0-2 cut off)
          uint8_t result3 = (lagLineCounter >> 3) & 0x07;
          result3++;
          
          //result3 = 7;
          while(result3) {
            __asm__ __volatile__ ("nop\n\t");
            __asm__ __volatile__ ("nop\n\t");
            __asm__ __volatile__ ("nop\n\t");
            __asm__ __volatile__ ("nop\n\t");
            __asm__ __volatile__ ("nop\n\t");
            __asm__ __volatile__ ("nop\n\t");
            __asm__ __volatile__ ("nop\n\t");
            __asm__ __volatile__ ("nop\n\t");
            RILT_PORT |= (1<<VID_PORT_PIN); // Set to white
            __asm__ __volatile__ ("nop\n\t");
            __asm__ __volatile__ ("nop\n\t");
            __asm__ __volatile__ ("nop\n\t");
            __asm__ __volatile__ ("nop\n\t");
            __asm__ __volatile__ ("nop\n\t");
            __asm__ __volatile__ ("nop\n\t");
            RILT_PORT &= ~(1<<VID_PORT_PIN); // Set to black
            result3--;
          }
        } else if( horizontalLineCount == RESULT4_START_LINE ) {

          // The result4 value is bits 2, 1, 0 (nothing cut off)
          uint8_t result4 = (lagLineCounter >> 6) & 0x07;
          result4++;
          
          //result4 = 7;
          while(result4) {
            __asm__ __volatile__ ("nop\n\t");
            __asm__ __volatile__ ("nop\n\t");
            __asm__ __volatile__ ("nop\n\t");
            __asm__ __volatile__ ("nop\n\t");
            __asm__ __volatile__ ("nop\n\t");
            __asm__ __volatile__ ("nop\n\t");
            __asm__ __volatile__ ("nop\n\t");
            __asm__ __volatile__ ("nop\n\t");
            RILT_PORT |= (1<<VID_PORT_PIN); // Set to white
            __asm__ __volatile__ ("nop\n\t");
            __asm__ __volatile__ ("nop\n\t");
            __asm__ __volatile__ ("nop\n\t");
            __asm__ __volatile__ ("nop\n\t");
            __asm__ __volatile__ ("nop\n\t");
            __asm__ __volatile__ ("nop\n\t");
            RILT_PORT &= ~(1<<VID_PORT_PIN); // Set to black
            result4--;
          }

          resultFramesToShow--;
          if(resultFramesToShow == 0) {
            // We've displayed results for 255 frames (approximately 4 seconds).
            // We set measuringLag to true, and drawBox to true
            measuringLag = true;
            drawBox = true;
            lagLineCounter = 0;
          }

        }
        
      }
      
    } else if(timerCycles == 1) {
      // We've gone through 1 timer cycle
      timerCycles++;
    }

    return;
  }


  // Set to sync inactive / black
  // Vid should already be 0
  RILT_PORT &= ~(1<<VID_PORT_PIN);
  RILT_PORT |= (1<<SYNC_PORT_PIN);

  // If we this is the first timer overflow, then we're in the Front Porch
  // for 1.5us (approx 25 clock ticks at 16.5Mhz)
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
  __asm__ __volatile__ ("nop\n\t"); // 23rd clock

  // We enter the sync pulse for 4.7us (approximately 78 clock ticks at 16.5Mhz)
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
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t"); // 67th clock




  // If we're in the VSync region (i.e. on line 4, 5, or 6 (3 FP, 3 SP, 16 BP) for 240p,
  // then keep sync LOW (the back porch is 0 during VSync)
  // The Back Porch is 1.5us, or approximately 25 clock ticks at 16.5Mhz.

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
      // The last things we do is reset the line counter, increment the timerCycle counter, and return
      horizontalLineCount = 1;
      timerCycles++;
      return;
    }

    // If we're in the region to draw the line white,
    // then wait the rest of the Back Porch duration, 
    // then draw the line white
    if( (horizontalLineCount > BOX_START_LINE) && (horizontalLineCount < BOX_END_LINE) ) {

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

      if(drawBox) {
        // set Vid to high (i.e. start drawing a white line of the box)
        RILT_PORT |= (1<<VID_PORT_PIN);
        // The Vid line will be set low during the next timer cycle (timer cycle 2)
      }
      
    }
    
  } else {
    // We're in the VSync region.
    // Keep sync line LOW.
    
  }

  // At this point, we're in the first timer cycle, and
  // we've drawn the Front Porch and Sync Pulse.
  // If we were drawing the the box lines, we're just after
  // the start of the active line (after back porch).
  // If we aren't drawing the box lines, we're near the start
  // of the back porch.  The Back Porch for 240p is approx 1.5us,
  // so we accept this margin of error.
  // If we're supposed to be measuring lag, then we check to see 
  // if the sensor has detected light.
  // If it has detected light, then we can set measuringLag to false,
  // 
  // we increment the lagLineCounter.
  if(measuringLag) {
    // We're checking if the SENSOR_PORT_PIN is LOW (light detected).
    uint8_t pin = RILT_PIN & (1<<SENSOR_PORT_PIN);
    if(lagLineCounter == 2620) {pin = 0;}
    if( pin == 0 ) {
      // Light was detected.
      // We are no longer measuring lag.
      measuringLag = false;
      // The value of lagLineCounter will be used
      // in another part of the code to show results.

      // We also don't need to draw the box.
      drawBox = false;

      // We set resultFramesToShow to 255
      resultFramesToShow = 255;
      
    } else {
      // Light was not detected.
      // Increment the lagLineCounter and move on.
      lagLineCounter++;
    }
  }

  timerCycles++;
  return;
}


void loop() {}

