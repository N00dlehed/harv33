#include "NeoPixelFX.h"

NeoPixelFX::NeoPixelFX(int pin, int numPixels) 
  : _strip(numPixels, pin, NEO_GRB + NEO_KHZ800) {
  _numPixels = numPixels;
  _breathPhase = 0;
}

void NeoPixelFX::begin() {
  _strip.begin();
  // POWER FIX: Lower brightness to prevent brownouts (15 is safe)
  _strip.setBrightness(15); 
  
  // FORCE INITIAL STATE: Off
  _strip.fill(_strip.Color(0, 0, 0));
  _strip.show();
}

void NeoPixelFX::update(HarvState state) {
  uint32_t targetColor;
  
  // 1. Calculate Breath (Sine wave)
  float speed = (state == STATE_IDLE) ? 0.05 : 0.2;
  _breathPhase += speed;
  
  // Scale brightness (5 to 15)
  int brightness = (sin(_breathPhase) + 1.0) * 5.0 + 5.0; 

  // 2. Determine Color based on State
  switch (state) {
    case STATE_IDLE:
      targetColor = _strip.Color(0, 0, brightness); // Blue
      break;
    case STATE_MOVING:
      targetColor = _strip.Color(0, brightness, 5); // Teal
      break;
    case STATE_TILTED:
      targetColor = _strip.Color(brightness, 0, 0); // Red
      break;
    case STATE_TOUCHED:
      targetColor = _strip.Color(brightness, 5, 15); // Purple
      break;
    case STATE_DIZZY:
      targetColor = _strip.Color(brightness, brightness, 0); // Yellow/Dizzy
      break;
    default:
      targetColor = _strip.Color(0,0,0);
      break;
  }

  _strip.setPixelColor(0, targetColor);
  _strip.show();
}