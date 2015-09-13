#include <avr/interrupt.h>
#include <avr/io.h>

#include <TVout.h>
#include <fontALL.h>

#include <SNESController.h>

#include <MusicalNoteFrequencies.h>
#include "triforce.h"

#define W 136
#define H 98

#define NOTE_LENGTH_MS 175
#define NOTE_WAIT_MS NOTE_LENGTH_MS

const int PROGMEM notes[] = {
  NOTE_A5,  NOTE_D5, NOTE_AS4, NOTE_G4,
  NOTE_G5,  NOTE_D5, NOTE_AS4, NOTE_G4,
  NOTE_FS5, NOTE_D5, NOTE_AS4, NOTE_G4,
  NOTE_G5,  NOTE_D5, NOTE_AS4, NOTE_G4,
  
  NOTE_G5,  NOTE_C5, NOTE_A4, NOTE_F4,
  NOTE_F5,  NOTE_C5, NOTE_A4, NOTE_F4,
  NOTE_E5,  NOTE_C5, NOTE_A4, NOTE_F4,
  NOTE_F5,  NOTE_C5, NOTE_A4, NOTE_F4,
  
  NOTE_F5,  NOTE_AS4, NOTE_G4, NOTE_E4,
  NOTE_E5,  NOTE_AS4, NOTE_G4, NOTE_E4,
  NOTE_DS5, NOTE_AS4, NOTE_G4, NOTE_E4,
  NOTE_E5,  NOTE_AS4, NOTE_G4, NOTE_E4,
  
  NOTE_E5,  NOTE_A4, NOTE_F4, NOTE_D4,
  NOTE_D5,  NOTE_A4, NOTE_F4, NOTE_D4,
  NOTE_CS5, NOTE_A4, NOTE_F4, NOTE_D4,
  NOTE_D5,  NOTE_A4, NOTE_F4, NOTE_D4,
  
  
  NOTE_A5,  NOTE_D5, NOTE_AS4, NOTE_G4,
  NOTE_G5,  NOTE_D5, NOTE_AS4, NOTE_G4,
  NOTE_FS5, NOTE_D5, NOTE_AS4, NOTE_G4,
  NOTE_G5,  NOTE_D5, NOTE_AS4, NOTE_G4,
  
  NOTE_AS5, NOTE_DS5, NOTE_C5, NOTE_F4,
  NOTE_A5,  NOTE_DS5, NOTE_C5, NOTE_F4,
  NOTE_GS5, NOTE_DS5, NOTE_C5, NOTE_F4,
  NOTE_A5,  NOTE_DS5, NOTE_C5, NOTE_F4,
  
  NOTE_C6,  NOTE_D5, NOTE_AS4, NOTE_G4,
  NOTE_AS5, NOTE_D5, NOTE_AS4, NOTE_G4,
  NOTE_A5,  NOTE_D5, NOTE_AS4, NOTE_G4,
  NOTE_AS5, NOTE_D5, NOTE_AS4, NOTE_G4,
  
  NOTE_A5,  NOTE_AS4, NOTE_G4, NOTE_E4,
  NOTE_G5,  NOTE_AS4, NOTE_G4, NOTE_E4,
  NOTE_F5,  NOTE_AS4, NOTE_G4, NOTE_E4,
  NOTE_E5,  NOTE_AS4, NOTE_G4, NOTE_E4
  
};


const int PROGMEM noteDurations[] = {
  
  150, 150, 150, 150,
  150, 150, 150, 150,
  150, 150, 150, 150,
  150, 150, 150, 150,
  
  150, 150, 150, 150,
  150, 150, 150, 150,
  150, 150, 150, 150,
  150, 150, 150, 150,
  
  150, 150, 150, 150,
  150, 150, 150, 150,
  150, 150, 150, 150,
  150, 150, 150, 150,
  
  150, 150, 150, 150,
  150, 150, 150, 150,
  150, 150, 150, 150,
  150, 150, 150, 150,
  
  
  100, 100, 100, 100,
  100, 100, 100, 100,
  100, 100, 100, 100,
  100, 100, 100, 100,
  
  100, 100, 100, 100,
  100, 100, 100, 100,
  100, 100, 100, 100,
  100, 100, 100, 100,

  150, 150, 150, 150,
  150, 150, 150, 150,
  150, 150, 150, 150,
  150, 150, 150, 150,
  
  150, 150, 150, 150,
  150, 150, 150, 150,
  150, 150, 150, 150,
  150, 150, 150, 150,
};


TVout tv;

// background music variables
uint16_t backgroundMusicCounter;
uint16_t noteCounter;

void updateBackgroundMusic() {

  // if the counter has been reset, we are ready to play our note
  if(backgroundMusicCounter == 0) {
    if(noteCounter >= (sizeof(notes) >> 1)) { // loops the background music
      noteCounter = 0;
    }
    
    tv.tone(pgm_read_word(notes + noteCounter), pgm_read_word(noteDurations + noteCounter));
    ++noteCounter;
  }
  
  if( (backgroundMusicCounter << 4) >= (uint16_t)pgm_read_word(noteDurations + noteCounter) ) {
    backgroundMusicCounter = 0;
  } else {
    ++backgroundMusicCounter;
  }
}

// position of the triforce bitmap
uint8_t triforceXpos;
uint8_t triforceYpos;

// game controller
SNESController gameController(2,3,4);


void move_triforce(uint16_t buttons) {
  
  tv.draw_rect(triforceXpos, triforceYpos, pgm_read_byte(triforce), pgm_read_byte(triforce + 1), BLACK);
  
  if(buttons & SNES_RIGHT) {
    if(triforceXpos < (W-1)) {
      ++triforceXpos;
    }
  }
 
  if (buttons & SNES_LEFT) {
    if(triforceXpos > 0) {
      --triforceXpos;
    }
  }
  
  if(buttons & SNES_DOWN) {
    if(triforceYpos < (H-1)) {
      ++triforceYpos;
    }
  }
 
  if (buttons & SNES_UP) {
    if(triforceYpos > 0) {
      --triforceYpos;
    }
  }

  tv.bitmap(triforceXpos, triforceYpos, triforce);
}


void setup() {
  
  backgroundMusicCounter = 0;
  noteCounter = 0;
  
  triforceXpos = 0;
  triforceYpos = 0;
  
  tv.begin(NTSC, W, H);  
  tv.fill(0);
  tv.bitmap(triforceXpos, triforceYpos, triforce);
  tv.select_font(font6x8);
  tv.print(80, 0, "Test");
}

int test=0;

void loop() {
  
  /*
  if(noteCounter >= (sizeof(notes) >> 1) ) {
    noteCounter = 0; 
  }
  
  tv.tone(pgm_read_word(notes + noteCounter), NOTE_LENGTH_MS);
  noteCounter++;
  
  tv.delay(NOTE_WAIT_MS);
  */
  
  updateBackgroundMusic();
  
  
  // read controller button data
  uint16_t gameControllerState = gameController.getState();
    
  // move the triforce image
  //move_triforce(gameControllerState);
  if(test < 60) {
    move_triforce(SNES_RIGHT | SNES_DOWN);
    test++;
  } else if(test < 120) {
    move_triforce(SNES_LEFT | SNES_UP);
    test++; 
  } else if(test >= 120) {
    test = 0; 
  }
  
  tv.delay_frame(1);
}
