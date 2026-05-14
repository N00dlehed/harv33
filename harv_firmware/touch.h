#pragma once

#define TOUCH_PIN        T0   // GPIO 4
#define TOUCH_THRESHOLD  65
#define TOUCH_COOLDOWN   1200 // ms between touch events

namespace Touch {
  unsigned long lastTouchTime = 0;

  void begin() {
    // ESP32 capacitive touch needs no setup
  }

  bool isPressed() {
    return touchRead(TOUCH_PIN) < TOUCH_THRESHOLD;
  }

  // Returns true once per touch event, respects cooldown
  bool poll() {
    if (!isPressed()) return false;
    unsigned long now = millis();
    if (now - lastTouchTime < TOUCH_COOLDOWN) return false;
    lastTouchTime = now;
    return true;
  }
}