#pragma once
#include <Adafruit_NeoPixel.h>

#define LED_PIN    16
#define LED_COUNT  1

Adafruit_NeoPixel led(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

namespace LED {
  uint8_t baseR = 20, baseG = 40, baseB = 255;
  float breathSpeed = 0.015;
  float breathMin   = 0.15;
  float breathVal   = 0;

  // Target for smooth color blending
  uint8_t fromR, fromG, fromB;
  uint8_t toR,   toG,   toB;
  float   blendT     = 1.0;
  float   blendSpeed = 0.02;

  unsigned long lastUpdate = 0;
  #define LED_INTERVAL 16  // ~60fps

  void begin() {
    led.begin();
    led.setBrightness(180);
    led.show();
    fromR = toR = baseR = 20;
    fromG = toG = baseG = 40;
    fromB = toB = baseB = 255;
  }

  void blendTo(uint8_t r, uint8_t g, uint8_t b, float speed = 0.015) {
    fromR = baseR; fromG = baseG; fromB = baseB;
    toR = r; toG = g; toB = b;
    blendT = 0.0;
    blendSpeed = speed;
  }

  void setState(const String& state) {
    if (state == "happy") {
      blendTo(255, 160, 50,  0.02);
      breathSpeed = 0.025; breathMin = 0.3;
    } else if (state == "excited") {
      blendTo(255, 220, 100, 0.04);
      breathSpeed = 0.05;  breathMin = 0.4;
    } else if (state == "warm" || state == "calm") {
      blendTo(255, 100, 20,  0.015);
      breathSpeed = 0.018; breathMin = 0.25;
    } else if (state == "bright") {
      blendTo(255, 180, 60,  0.02);
      breathSpeed = 0.03;  breathMin = 0.35;
    } else if (state == "dim") {
      blendTo(5,   10,  60,  0.008);
      breathSpeed = 0.006; breathMin = 0.05;
    } else if (state == "scared") {
      blendTo(180, 180, 255, 0.05);
      breathSpeed = 0.07;  breathMin = 0.1;
    } else if (state == "anxious") {
      blendTo(255, 70,  0,   0.03);
      breathSpeed = 0.045; breathMin = 0.2;
    } else if (state == "curious") {
      blendTo(80,  200, 255, 0.025);
      breathSpeed = 0.035; breathMin = 0.3;
    } else {
      // idle — soft blue
      blendTo(20,  40,  255, 0.02);
      breathSpeed = 0.018; breathMin = 0.04;
    }
  }

  void update() {
    unsigned long now = millis();
    if (now - lastUpdate < LED_INTERVAL) return;
    lastUpdate = now;

    // Blend toward target color (signed to avoid uint8_t wrap on downward transitions)
    if (blendT < 1.0) {
      blendT = min(1.0f, blendT + blendSpeed);
      baseR = (uint8_t)((int16_t)fromR + (int16_t)(((int16_t)toR - (int16_t)fromR) * blendT));
      baseG = (uint8_t)((int16_t)fromG + (int16_t)(((int16_t)toG - (int16_t)fromG) * blendT));
      baseB = (uint8_t)((int16_t)fromB + (int16_t)(((int16_t)toB - (int16_t)fromB) * blendT));
    }

    // Breathing sine wave
    breathVal += breathSpeed;
    if (breathVal > TWO_PI) breathVal -= TWO_PI;
    float brightness = breathMin + (1.0 - breathMin) * (0.5 + 0.5 * sin(breathVal));

    led.setPixelColor(0, led.Color(
      (uint8_t)(baseR * brightness),
      (uint8_t)(baseG * brightness),
      (uint8_t)(baseB * brightness)
    ));
    led.show();
  }
}