#include "Display.h"

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

#define EYE_W 40      
#define EYE_H 45      
#define EYE_GAP 12    
#define CORNER_RAD 8  

#define LEFT_EYE_CX (SCREEN_WIDTH/2 - EYE_W/2 - EYE_GAP/2)
#define RIGHT_EYE_CX (SCREEN_WIDTH/2 + EYE_W/2 + EYE_GAP/2)
#define EYE_CY 40

Display::Display() : _oled(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET) {
  _isBlinking = false;
  _pupilX = 0;
  _pupilY = 0;
  _nextBlinkTime = 0;
}

void Display::begin() {
  if(!_oled.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println(F("SSD1306 allocation failed"));
  }
  _oled.clearDisplay();
  _oled.display();
}

void Display::showBootScreen() {
  _oled.clearDisplay();
  _oled.setTextSize(2);
  _oled.setTextColor(SSD1306_WHITE);
  int x = SCREEN_WIDTH/2 - 20;
  int y = 40 - 10; 
  _oled.fillRoundRect(x, y, 40, 20, 5, SSD1306_WHITE);
  _oled.setTextColor(SSD1306_BLACK);
  _oled.setCursor(x + 5, y + 3);
  _oled.print(F("^_^"));
  _oled.display();
}

void Display::update(HarvState state) {
  _oled.clearDisplay();
  updatePhysics(state);

  switch(state) {
    case STATE_IDLE:    drawNeutralEyes(); break;
    case STATE_MOVING:  drawFocusedEyes(); break;
    case STATE_TILTED:  drawPanicEyes(); break;
    case STATE_TOUCHED: drawHappyEyes(); break;
    case STATE_SLEEPING: drawSleepingEyes(); break;
    case STATE_DIZZY:    drawDizzyEyes(); break;
  }
  _oled.display();
}

void Display::updatePhysics(HarvState state) {
  unsigned long now = millis();
  
  // Blink Logic
  if (_isBlinking) {
    if (now - _blinkStart > 100) {
      _isBlinking = false;
      _nextBlinkTime = now + random(1000, 5000);
    }
  } else {
    if (now > _nextBlinkTime && state == STATE_IDLE) {
      _isBlinking = true;
      _blinkStart = now;
    }
  }
  
  // Gaze Logic
  if (state == STATE_IDLE) { 
    if (now - _lastGazeMove > _nextGazeInterval) {
      _pupilX = random(-5, 6); 
      _pupilY = random(-3, 4); 
      _lastGazeMove = now;
      _nextGazeInterval = random(500, 3000); 
    }
  } else {
    _pupilX = 0; _pupilY = 0;
  }
}

void Display::drawVectorEye(int x_center, int y_center, int w, int h, float eyelid_top, float eyelid_bottom, int offset_x, int offset_y, bool isLeft) {
  int x = x_center - w/2 + offset_x;
  int y = y_center - h/2 + offset_y;
  _oled.fillRoundRect(x, y, w, h, CORNER_RAD, SSD1306_WHITE);
  if (eyelid_top > 0) {
    int lid_h = (int)(h * eyelid_top);
    _oled.fillRect(x, y - 2, w, lid_h + 2, SSD1306_BLACK);
  }
  if (eyelid_bottom > 0) {
    int lid_h = (int)(h * eyelid_bottom);
    _oled.fillRect(x, y + h - lid_h, w, lid_h + 2, SSD1306_BLACK);
  }
}

void Display::drawNeutralEyes() {
  if (_isBlinking) {
    _oled.fillRect(LEFT_EYE_CX - EYE_W/2, EYE_CY, EYE_W, 2, SSD1306_WHITE);
    _oled.fillRect(RIGHT_EYE_CX - EYE_W/2, EYE_CY, EYE_W, 2, SSD1306_WHITE);
    return;
  }
  drawVectorEye(LEFT_EYE_CX, EYE_CY, EYE_W, EYE_H, 0.0, 0.0, _pupilX, _pupilY, true);
  drawVectorEye(RIGHT_EYE_CX, EYE_CY, EYE_W, EYE_H, 0.0, 0.0, _pupilX, _pupilY, false);
}

void Display::drawFocusedEyes() {
  int w = EYE_W - 4; int h = EYE_H; 
  drawVectorEye(LEFT_EYE_CX, EYE_CY, w, h, 0.25, 0.25, 0, 0, true);
  drawVectorEye(RIGHT_EYE_CX, EYE_CY, w, h, 0.25, 0.25, 0, 0, false);
}

void Display::drawHappyEyes() {
  drawVectorEye(LEFT_EYE_CX, EYE_CY, EYE_W, EYE_H, 0.0, 0.6, 0, 0, true);
  drawVectorEye(RIGHT_EYE_CX, EYE_CY, EYE_W, EYE_H, 0.0, 0.6, 0, 0, false);
}

void Display::drawPanicEyes() {
  int jitterX = random(-2, 3); int jitterY = random(-2, 3);
  int w = EYE_W - 10; int h = EYE_H - 10;
  drawVectorEye(LEFT_EYE_CX, EYE_CY, w, h, 0.0, 0.0, jitterX, jitterY, true);
  drawVectorEye(RIGHT_EYE_CX, EYE_CY, w, h, 0.0, 0.0, jitterX, jitterY, false);
}

void Display::drawSleepingEyes() {
  _oled.fillRect(LEFT_EYE_CX - EYE_W/2, EYE_CY + 10, EYE_W, 3, SSD1306_WHITE);
  _oled.fillRect(RIGHT_EYE_CX - EYE_W/2, EYE_CY + 10, EYE_W, 3, SSD1306_WHITE);
  if ((millis() / 1000) % 2 == 0) {
    _oled.setCursor(SCREEN_WIDTH/2 - 5, 10);
    _oled.setTextSize(1);
    _oled.setTextColor(SSD1306_WHITE);
    _oled.print("zZ");
  }
}

void Display::drawDizzyEyes() {
  for(int r=2; r<15; r+=4) {
    _oled.drawCircle(LEFT_EYE_CX, EYE_CY, r, SSD1306_WHITE);
    _oled.drawCircle(RIGHT_EYE_CX, EYE_CY, r, SSD1306_WHITE);
  }
}