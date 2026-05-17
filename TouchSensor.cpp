#include "TouchSensor.h"

TouchSensor::TouchSensor(int pin, int threshold) {
  _pin = pin;
  _threshold = threshold;
}

void TouchSensor::begin() {
  // ESP32 touch pins don't need a specific pinMode() call for touchRead,
  // but attaching an interrupt is a common way to 'wake' the pin if used for deep sleep.
  // For standard loop polling, we just ensure it's in a good state.
  // The empty ISR is just a placeholder if we were to use interrupts.
  touchAttachInterrupt(_pin, []{}, _threshold); 
}

bool TouchSensor::isTouched() {
  // touchRead returns a lower value when touched.
  // Typical open reading is > 50-70. Touched is often < 20-30.
  return (readRaw() < _threshold);
}

int TouchSensor::readRaw() {
  return touchRead(_pin);
}