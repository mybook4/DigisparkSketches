/////////////////////////////////////////////////////////////////////////////////////
// The Ridiculously Inexpensive Lag Tester (RILT) - Arduino Pro Mini 3.3v edition
//
// Note:  This is the 240p/480p Y (luma) version (Y of YPbPbr) for the Pro Mini.
// The Arduino Pro Mini (8Mhz) is used (which has an ATMega328p).
//
// Port B is used.
//
// PB0 - Negative Sync Active (0v), Negative Sync Inactive (3.3v)
// PB1 - Black (0v), White (3.3v)
//       PB0 and PB1 are used to generate the Y signal, which has built-in sync
// 
//       Sync
//       PB0-----/\/\/\/------|
//               560 Ohm      |
//       B/W                  |
//       PB1-----/\/\/\/------*-----Output To Display (Vo)... ---/\/\/\/---GND
//               270 Ohm                                         75 Ohm (term)
//                      
//
//       PB1 | PB0 | ~Vo (terminated)
//       --------------------------
//       0v  |  0v | 0v (in sync pulse)
//       0v  |3.3v | 0.313v (Black)
//     3.3v  |  0v | 0.649v (Grey)
//     3.3v  |3.3v | 0.962v (White)
//
//       Vo (terminated) = ((Vpb1/Rpb1) + (Vpb0/Rpb0)) / ((1/75)+(1/Rpb0)+(1/Rpb10)
// 
// PB2 - Connection to light sensor used to detect presence of white on the display.
//       PB2 requires a fast photodiode (e.g. recommend quicker than 100us response)
//       I am planning to use a Vishay TEFD4300 (100ns rise/fall time) to control
//       a transistor (2N3904).  This circuit needs to be reviewed for resistor
//       design accuracy.  THIS HAS NOT BEEN TESTED YET.
//
//                    VCC
//                     |
//                     |---------/\/\/\/--------*----PB2
//                     |         5k Ohm         |
//                     |                        |
//                     |                       /
//                     |---|>|-------/\/\/\/--|   2N3904
//                        Photo      50k Ohm   \
//                        diode                 |
//                     (TEFD4300)               |
//                                             GND
//  
//
// PB3 - Mode select (240p/480p)
//       Pull PB3 to ground for 480p mode. (NOT IMPLEMENTED YET)
//       Leave PB3 pulled up to 5v for 240p mode (ATMega328p internal pullup enabled).
// 
// 
// Concept: The goal was to create an inexpensive analog display lag testing device.
//          The design consists of a small microcontroller board (Arduino Pro Mini),
//          a relatively fast photodiode (for display light detection), 
//          a couple of resistors, a common transistor, and a single RCA connector 
//          for analog video output.
//          Since the ATMega328P has a 16-bit timer (Timer1), we use this.
//          We set up Timer1 for CTC mode, and set the OCR1A value appropriately. 
//          
//
/////////////////////////////////////////////////////////////////////////////////////

#define RILT_PORT PORTB
#define RILT_PIN  PINB
#define RILT_DDR  DDRB

#define SYNC_PORT_PIN 0    // PB0
#define VID_PORT_PIN  1    // PB1
#define SENSOR_PORT_PIN 2  // PB2
#define VIDMODE_PORT_PIN 3 // PB3

#define BOX_START_LINE_240P 125
#define BOX_END_LINE_240P 146

#define BOX_START_LINE_480P 265
#define BOX_END_LINE_480P 305

#define VIDMODE_240P 0
#define VIDMODE_480P 1

#define RESULT_WAIT_IN_LINES 62492

#define OCR1A_VALUE_FOR_240P 507  // 8,000,000 / (507+1) = 15,748 Hz
#define OCR1A_VALUE_FOR_480P 253  // 8,000,000 / (253+1) = 31,496 Hz

uint8_t vidMode;

uint16_t horizontalLineCount;

bool measuringLag;
bool drawBox;
uint16_t lagLineCounter;
uint16_t resultLinesWaited;

bool printSerialFlag;

void setup() {

  // Initialize GPIO
  
  // Set our Sync and Video pins to output mode
  RILT_DDR |= ( (1<<SYNC_PORT_PIN) | (1<<VID_PORT_PIN) );

  // Set our Mode and Sensor pins to input mode
  RILT_DDR &= ~( (1<<VIDMODE_PORT_PIN) | (1<<SENSOR_PORT_PIN) ); 

  // Set our Sync and Video pins to an initial value of sync inactive / black
  RILT_PORT &= ~(1<<VID_PORT_PIN);
  RILT_PORT |= (1<<SYNC_PORT_PIN);

  // Activate pullup resistors for the Mode and Sensor pins.
  RILT_PORT |= ( (1<<VIDMODE_PORT_PIN) | (1<<SENSOR_PORT_PIN) );
  

  // Initialize Serial and send a welcome message
  Serial.begin(9600);
  Serial.println("Welcome to RILT:  The Ridiculously Inexpensive Lag Tester");
  Serial.println("                  (3.3v Arduino Pro Mini Edition)");

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
    Serial.println("Operating mode: 240p");
    Serial.println("  To obtain time (in milliseconds), multiply the lines of lag reported");
    Serial.println("  by 63.55us per line, then multiply by 1,000 (1,000us in 1ms).");
    
  } else {
    // We're in 480p mode (the Mode pin was low)
    vidMode = VIDMODE_480P;
    OCR1A = OCR1A_VALUE_FOR_480P;
    Serial.println("Operating mode: 480p (not implemented yet!)");
    Serial.println("  To obtain time (in milliseconds), multiply the lines of lag reported");
    Serial.println("  by 31.77us per line, then multiply by 1,000 (1,000us in 1ms).");
  }

  // Continue initializing Timer1
  TCCR1B |= (1<<WGM12);  // Turn on CTC mode
  TCCR1B |= (1<<CS10);   // clk/1 prescaler
  TIMSK1 |= (1<<OCIE1A); // enable timer compare interrupt

  horizontalLineCount = 1;
  drawBox = true;
  measuringLag = true;
  lagLineCounter = 0;
  resultLinesWaited = 0;
  printSerialFlag = false;

  sei();
}



////////////////////////////// Implementation which generates 240p //////////////////////////
void generate240p() {

  // Set to sync inactive / black
  // Vid should already be 0
  RILT_PORT &= ~(1<<VID_PORT_PIN);
  RILT_PORT |= (1<<SYNC_PORT_PIN);

  // We're in the Front Porch region
  // for 1.5us (approx 12 clock ticks at 8Mhz)
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t"); // 8 clocks (overhead) 

  // We enter the sync pulse for 4.7us (approximately 38 clock ticks at 8Mhz)
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
  __asm__ __volatile__ ("nop\n\t"); // 26th clock (overhead)


  // If we're in the VSync region (i.e. on line 4, 5, or 6 (3 FP, 3 SP, 16 BP) for 240p,
  // then keep sync LOW (the back porch is 0 during VSync)
  // The Back Porch is 1.5us, or approximately 12 clock ticks at 8Mhz.

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
      return;
    }

    // If we're in the region to draw the line white,
    // then wait the rest of the Back Porch duration, 
    // then draw the line white
    if( (horizontalLineCount > BOX_START_LINE_240P) && (horizontalLineCount < BOX_END_LINE_240P) ) {

      // Waiting for back porch to complete (approximately 12 clocks minus overhead)
      __asm__ __volatile__ ("nop\n\t"); // 1 clock (overhead)

      if(drawBox) {
        // set Vid to high (i.e. start drawing a white line of the box)
        RILT_PORT |= (1<<VID_PORT_PIN);

        // Wait for 25% of the screen to turn Vid back to black
        // The visible line time for 240p is 
        // 63.55us -1.5us(FP) -4.7us(Sync) -1.5us(BP) = 55.85us.
        // 0.25 * 55.85us = 13.96us
        // 13.96us / 0.125 = 112 cycles
        // Doesn't have to be exact, just enough to make a visible white box.
        uint8_t waitCounter = 10;
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

      // set Vid to high (i.e. start drawing a white line of the box)
      RILT_PORT |= (1<<VID_PORT_PIN);
    }
    
  } else {
    // We're in the VSync region.
    // Keep sync line LOW.
    
  }


  // At this point, we've drawn the Front Porch and Sync Pulse.
  // If we were drawing the the box lines, we're just after
  // the end of the box line being drawn (when the line 
  // has been turned from white to black).
  // If we aren't drawing the box lines, we're near the start
  // of the back porch.  The Back Porch for 240p is approx 1.5us,
  // and drawing the white line of the box adds approx 13.96us,
  // so we accept this margin of error.
  // If we're supposed to be measuring lag, then we check to see 
  // if the sensor has detected light.
  // If it has detected light, then we can set measuringLag to false,
  // 
  // we increment the lagLineCounter.
  if(measuringLag) {
    // We're checking if the SENSOR_PORT_PIN is LOW (light detected).
    uint8_t pin = RILT_PIN & (1<<SENSOR_PORT_PIN);
    if(lagLineCounter == 1310) {pin = 0;}  // TODO: REMOVE ONCE TESTING IS COMPLETE WITH PHOTODIODE
    if( pin == 0 ) {
      // Light was detected.
      // We are no longer measuring lag.
      measuringLag = false;
      // The value of lagLineCounter will be used
      // in another part of the code to show results.

      // We also don't need to draw the box.
      drawBox = false;
      
    } else {
      // Light was not detected.
      // Increment the lagLineCounter and move on.
      lagLineCounter++;
    }
    
  } else {
    // We're not measuring lag.
    // This means that light was detected.
    // We print the result over serial, then wait for 
    // the number of lines designated by resultLinesWaited 
    // and reset values

    if(resultLinesWaited == RESULT_WAIT_IN_LINES) {
      // We've finished waiting.
      // We set measuringLag to true, drawBox to true,
      // and reset the lagLineCounter
      measuringLag = true;
      drawBox = true;
      lagLineCounter = 0;
      // We reset resultWaitInLines
      resultLinesWaited = 0;
      
    } else {
      
      if(resultLinesWaited == 0) {
        printSerialFlag = true;
        //Serial.print(lagLineCounter);
        //Serial.print(" lines of lag detected\n");
      }
      
      resultLinesWaited++;
    }

    
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
  // for 0.6355us (approx 5 clock ticks at 8Mhz)
  //__asm__ __volatile__ ("nop\n\t"); // 1st clock (overhead)

  // We enter the sync pulse for 3.8133us (approximately 31 clock ticks at 8Mhz)
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
  __asm__ __volatile__ ("nop\n\t"); // 18th clock (overhead)


  // If we're in the VSync region (i.e. on line 11, 12, or 13 (10 FP, 3 SP, 32 BP) for 480p,
  // then keep sync LOW (the back porch is 0 during VSync)
  // The Back Porch is 1.9066us, or approximately 15 clock ticks at 8Mhz.

  // Note: The if() is structured this way to descrease jitter for 
  // when we're not in the VSync region.  The idea is that all parts
  // of the AND would need to be evaluated to true.  This might need to be 
  // replaced with assembly to prevent compiler optimizations from getting in
  // our way of our goal to reduce jitter.
  
  if( (horizontalLineCount != 11) && (horizontalLineCount != 12) && (horizontalLineCount != 13) ) {
    
    // We're not in the VSync region
    
    // Set to sync inactive / black
    RILT_PORT |= (1<<SYNC_PORT_PIN);
    
    if(horizontalLineCount == 525) {
      // We've reached the last line
      // The last things we do is reset the line counter and return
      horizontalLineCount = 1;
      return;
    }

    // If we're in the region to draw the line white,
    // then wait the rest of the Back Porch duration, 
    // then draw the line white
    if( (horizontalLineCount > BOX_START_LINE_480P) && (horizontalLineCount < BOX_END_LINE_480P) ) {

      // Waiting for back porch to complete (approximately 15 clocks minus overhead)
      //__asm__ __volatile__ ("nop\n\t"); // 1 clock (overhead)

      if(drawBox) {
        // set Vid to high (i.e. start drawing a white line of the box)
        RILT_PORT |= (1<<VID_PORT_PIN);

        // Wait for 25% of the screen to turn Vid back to black
        // The visible line time for 480p is 
        // 31.77us -0.6355us(FP) -3.8133us(Sync) -1.9066us(BP) = 25.42us.
        // 0.25 * 25.42us = 6.355us
        // 6.355us / 0.125 = 50 cycles
        // Doesn't have to be exact, just enough to make a visible white box.
        uint8_t waitCounter = 5;
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
      }
      
    } else if(horizontalLineCount == 75) {
      // draw a pilot line
      // Waiting for back porch to complete (approximately 12 clocks minus overhead)
      //__asm__ __volatile__ ("nop\n\t"); // 10th clock

      // set Vid to high (i.e. start drawing a white line of the box)
        RILT_PORT |= (1<<VID_PORT_PIN);
    }
    
  } else {
    // We're in the VSync region.
    // Keep sync line LOW.
    
  }


  // At this point, we've drawn the Front Porch and Sync Pulse.
  // If we were drawing the the box lines, we're just after
  // the end of the box line being drawn (when the line 
  // has been turned from white to black).
  // If we aren't drawing the box lines, we're near the start
  // of the back porch.  The Back Porch for 480p is approx 1.9066us,
  // and drawing the white line of the box adds approx 6.355us,
  // so we accept this margin of error.
  // If we're supposed to be measuring lag, then we check to see 
  // if the sensor has detected light.
  // If it has detected light, then we can set measuringLag to false,
  // 
  // we increment the lagLineCounter.
  if(measuringLag) {
    // We're checking if the SENSOR_PORT_PIN is LOW (light detected).
    uint8_t pin = RILT_PIN & (1<<SENSOR_PORT_PIN);
    if(lagLineCounter == 1575) {pin = 0;}  // TODO: REMOVE ONCE TESTING IS COMPLETE WITH PHOTODIODE
    if( pin == 0 ) {
      // Light was detected.
      // We are no longer measuring lag.
      measuringLag = false;
      // The value of lagLineCounter will be used
      // in another part of the code to show results.

      // We also don't need to draw the box.
      drawBox = false;
      
    } else {
      // Light was not detected.
      // Increment the lagLineCounter and move on.
      lagLineCounter++;
    }
    
  } else {
    // We're not measuring lag.
    // This means that light was detected.
    // We print the result over serial, then wait for 
    // the number of lines designated by resultLinesWaited 
    // and reset values

    if(resultLinesWaited == RESULT_WAIT_IN_LINES) {
      // We've finished waiting.
      // We set measuringLag to true, drawBox to true,
      // and reset the lagLineCounter
      measuringLag = true;
      drawBox = true;
      lagLineCounter = 0;
      // We reset resultWaitInLines
      resultLinesWaited = 0;
      
    } else {
      
      if(resultLinesWaited == 0) {
        printSerialFlag = true;
        //Serial.print(lagLineCounter);
        //Serial.print(" lines of lag detected\n");
      }
      
      resultLinesWaited++;
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
  //sleep_mode();
  if(printSerialFlag) {
    Serial.print(lagLineCounter);
    Serial.print(" lines of lag detected\n");
    printSerialFlag = false;
  }
}

