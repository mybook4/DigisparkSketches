#include <TVout.h>
#include <fontALL.h>
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

TVout tv;
int noteCounter;


void setup() {
  noteCounter = 0;
  tv.begin(NTSC, W, H);  
  tv.fill(0);
  tv.bitmap(43, 24, triforce);
  tv.select_font(font6x8);
  tv.print(20, 80, "Zelda Music Test");
}

void loop() {
  
  if(noteCounter >= (sizeof(notes) >> 1) ) {
    noteCounter = 0; 
  }
  
  tv.tone(pgm_read_word(notes + noteCounter), NOTE_LENGTH_MS);
  noteCounter++;
  
  tv.delay(NOTE_WAIT_MS);
}
