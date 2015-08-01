/*
  Pong
  Copyright (C) 2011 nootropic design, LLC
  All rights reserved.
 
  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  Requires TVout library at Google Code:
  http://code.google.com/p/arduino-tvout/downloads/detail?name=TVoutBeta1.zip

  Requires Hackvision Controllers library:
  http://nootropicdesign.com/downloads/Controllers.zip
*/
#include <avr/pgmspace.h>
#include <TVout.h>
#include <video_gen.h>
#include <fontALL.h>
#include <Controllers.h>
#include "title_bitmap.h"
#include "pong_font.h"
#define W 136
#define H 98

char s[16]; // general string buffer
int pauseCounter = 0;

int ballx, bally;
char dx;
char dy;
byte paddleAy = 44;
byte paddleBy = 44;
byte paddleAx = 2;
byte paddleBx = W-5;
byte paddleWidth = 2;
byte paddleLength = 8;
byte score;
byte score2 = 0;
boolean useButtons = false;

TVout tv;
char initials[3];
boolean started = false;


void setup()  {
  // If pin 12 is pulled LOW, then the PAL jumper is shorted.
  pinMode(12, INPUT);
  digitalWrite(12, HIGH);

  if (digitalRead(12) == LOW) {
    tv.begin(PAL, W, H);
  } else {
    tv.begin(NTSC, W, H);
  }

  randomSeed(analogRead(0));
}


void titleScreen() {
  tv.fill(0);
  tv.bitmap(28, 30, title_bitmap);
  tv.select_font(font6x8);
  tv.print(24, 88, "ATARI INC. 1972");
}


void loop() {
  if (!started) {
    titleScreen();
    while (!Controller.firePressed()); // wait for coin
    tv.fill(0);
    initPong();
    drawNet();
    drawPaddles();
    drawScore();
    drawBall();
    pauseCounter = 240;
    started = true;
  }

  drawNet();
  drawPaddles();

  if (pauseCounter-- > 0) {
    // Delay while letting players move their paddles.
    // Ball will serve when pauseCounter reaches 0.
    tv.delay_frame(1);
    return;
  }

  eraseBall();
  moveBall();
  drawBall();

  tv.delay_frame(1);
}

void eraseBall() {
  tv.set_pixel(ballx, bally, 0);
  tv.set_pixel(ballx+1, bally, 0);
  tv.set_pixel(ballx, bally+1, 0);
  tv.set_pixel(ballx+1, bally+1, 0);
}

void drawBall() {
  tv.set_pixel(ballx, bally, 1);
  tv.set_pixel(ballx+1, bally, 1);
  tv.set_pixel(ballx, bally+1, 1);
  tv.set_pixel(ballx+1, bally+1, 1);
}

void drawPaddles() {
  int p;

  if (!useButtons) {
    // If the player presses the up or down buttons, then use
    // the button controller instead of paddle controllers.
    if (Controller.upPressed() || Controller.downPressed()) {
      useButtons = true;
      paddleBy = paddleAy;
    }
  }

  if (useButtons) {
    // The player is using the button controller.
    if ((Controller.upPressed()) && (paddleAy > 1)) {
      paddleAy = paddleAy - 2;
      paddleBy = paddleAy;
    }
    if ((Controller.downPressed()) && (paddleAy < (H-paddleLength-1))) {
      paddleAy = paddleAy + 2;
      paddleBy = paddleAy;
    }
  } else {
    // The player is using the paddle controllers.
    p = PaddleA.getPosition();
    paddleAy = map(p, 0, 1023, H-paddleLength, 0);
    p = PaddleB.getPosition();
    paddleBy = map(p, 0, 1023, H-paddleLength, 0);
  }

  drawPaddle(paddleAx, paddleAy);
  drawPaddle(paddleBx, paddleBy);
}

void drawScore() {
  byte x,y;
  for(y=0;y<7;y++){
    for(x=32;x<=40;x++) {
      tv.set_pixel(x, y, 0);
    }
    for(x=96;x<=104;x++) {
      tv.set_pixel(x, y, 0);
    }
  }
  tv.print_char(32, 0, '0'+score);
  tv.print_char(96, 0, '0'+score2);
}

void moveBall() {
  // detect score
  if (ballx <= 0) {
    score2++;
    drawScore();
    drawNet();
    scoreSound();
    if (score2 == 9) {
      gameOver();
      return;
    }
    ballx = random(W/2, (3*W)/4);
    bally = random(H/4, (3*H)/4);
    dx = -1;
    if (random(0, 2) == 0) {
      dy = 1;
    } else {
      dy = -1;
    }
    pauseCounter = 60;  // pause after score.  60 frames is 1s
  }
  if (ballx >= (W-2)) {
    score++;
    drawNet();
    drawScore();
    scoreSound();
    if (score == 9) {
      gameOver();
      return;
    }
    ballx = random(W/4, W/2);
    bally = random(H/4, (3*H)/4);
    dx = 1;
    if (random(0, 2) == 0) {
      dy = 1;
    } else {
      dy = -1;
    }
    pauseCounter = 60;
  }

  // detect hit
  if (ballx == paddleAx+paddleWidth) {
    if ((bally >= paddleAy) && (bally < (paddleAy+paddleLength))) {
      dx = 1;
      drawScore();
      hitSound();
    }
  }

  // detect hit
  if (ballx == paddleBx-2) {
    if ((bally >= paddleBy) && (bally < (paddleBy+paddleLength))) {
      dx = -1;
      drawScore();
      hitSound();
    }
  }

  if (bally <= 0) {
    bounceSound();
    dy = 1;
    drawScore();
  }
  if (bally >= (H-2)) {
    bounceSound();
    dy = -1;
    drawScore();
  }
 
  ballx = ballx + dx;
  bally = bally + dy;
}


void drawPaddle(int x, int y) {
  for(int i=x;i<x+paddleWidth;i++){
    tv.draw_line(i, 0, i, H-1, 0);
    tv.draw_line(i, y, i, y+paddleLength, 1);
  }
}

void hitSound() {
  tv.tone(932, 20);
}

void bounceSound() {
  tv.tone(458, 20);
}

void scoreSound() {
  tv.tone(987, 400);
}

void winSound() {
  tv.tone(987, 400);
}

void drawNet() {
  for(byte y=0;y<H-4;y=y+6) {
    tv.draw_line(W/2, y, W/2, y+2, 1);
  }
}

void gameOver() {
  tv.delay(1000);
  tv.fill(0);
  tv.select_font(font6x8);
  tv.print(44, 40, "GAME");
  tv.print(72, 40, "OVER");
  tv.delay(3000);
  started = false;
}


boolean pollFireButton(int n) {
  for(int i=0;i<n;i++) {
    tv.delay_frame(1);
    if (Controller.firePressed()) {
      return true;
    }
  }
  return false;
}


void initPong() {
  tv.select_font(pong_font);
  ballx = random(40, 57);
  bally = random(56, 73);
  dx = 1;
  if (random(0, 2) == 0) {
    dy = 1;
  } else {
    dy = -1;
  }
  score = 0;
  score2 = 0;
}

int getMemory() {
  int size = 512;
  byte *buf;
  while ((buf = (byte *) malloc(--size)) == NULL);
  free(buf);
  return size;
} 

