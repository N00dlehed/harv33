#pragma once
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

#define EYE_W 36
#define EYE_H 36
#define EYE_RADIUS 6
#define LEFT_EYE_X 32
#define RIGHT_EYE_X 96
#define EYE_Y 40

extern Adafruit_SSD1306 display;

// ── Debug bar (yellow zone, top 16px) ─────────────────────────
void debugBar(const String& msg) {
  display.fillRect(0, 0, SCREEN_WIDTH, 16, BLACK);
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(2, 4);
  display.print(msg);
  display.display();
}

// ── Core eye draw (only redraws blue zone) ────────────────────
void drawEyes(int leftY, int rightY, int leftH, int rightH, int offsetX = 0) {
  display.fillRect(0, 16, SCREEN_WIDTH, SCREEN_HEIGHT - 16, BLACK);
  display.fillRoundRect(LEFT_EYE_X  - EYE_W/2 + offsetX, leftY  - leftH/2,  EYE_W, leftH,  EYE_RADIUS, WHITE);
  display.fillRoundRect(RIGHT_EYE_X - EYE_W/2 + offsetX, rightY - rightH/2, EYE_W, rightH, EYE_RADIUS, WHITE);
  display.display();
}

// ── Expressions ───────────────────────────────────────────────

void faceIdle() {
  drawEyes(EYE_Y, EYE_Y, EYE_H, EYE_H);
}

void faceBlink() {
  for (int h = EYE_H; h > 4; h -= 8) { drawEyes(EYE_Y, EYE_Y, h, h); delay(30); }
  delay(60);
  for (int h = 4; h <= EYE_H; h += 8) { drawEyes(EYE_Y, EYE_Y, h, h); delay(30); }
}

void faceLookLeft() {
  drawEyes(EYE_Y, EYE_Y, EYE_H, EYE_H, -6);
  delay(800);
  faceIdle();
}

void faceLookRight() {
  drawEyes(EYE_Y, EYE_Y, EYE_H, EYE_H, 6);
  delay(800);
  faceIdle();
}

void faceHappy() {
  // Eyes squish upward — warm squint
  for (int h = EYE_H; h > 12; h -= 4) { drawEyes(EYE_Y + 6, EYE_Y + 6, h, h); delay(20); }
  delay(600);
  for (int h = 12; h <= EYE_H; h += 4) { drawEyes(EYE_Y, EYE_Y, h, h); delay(20); }
}

void faceSurprised() {
  // Eyes go wide
  drawEyes(EYE_Y, EYE_Y, EYE_H + 8, EYE_H + 8);
  delay(400);
  faceIdle();
}

void faceSleepy() {
  // Half closed eyes
  drawEyes(EYE_Y + 6, EYE_Y + 6, EYE_H / 2, EYE_H / 2);
}

void faceSad() {
  // Eyes droop down slightly, narrowed
  for (int h = EYE_H; h > 18; h -= 4) { drawEyes(EYE_Y + 4, EYE_Y + 4, h, h); delay(20); }
}

// ── Touch response — intensity driven by psyche ───────────────
// warmth: 0.0 - 1.0 from psyche state
void faceTouched(float warmth) {
  debugBar("touch <3");
  if (warmth > 0.65) {
    faceSurprised();
    delay(100);
    faceHappy();
  } else if (warmth > 0.35) {
    faceHappy();
  } else {
    // Low energy — just a slow blink
    faceBlink();
  }
}