#include "MotionSensor.h"
#include "Display.h"
#include "NeoPixelFX.h"
#include "StateMachine.h"

// --- PIN DEFINITIONS ---
#define PIN_LED     16
#define PIN_TOUCH   13 

// --- MODULES ---
MotionSensor  ears;
Display       face;
NeoPixelFX    eye(PIN_LED, 1);
StateMachine  brain;

void setup() {
  Serial.begin(115200);
  
  // POWER STABILIZATION DELAY
  delay(500); 
  
  ears.begin();
  face.begin();
  eye.begin();
  
  // Touch setup (Using empty ISR)
  touchAttachInterrupt(PIN_TOUCH, []{}, 40); 
  
  face.showBootScreen();
  delay(1000);
}

void loop() {
  // 1. SENSE
  ears.update();
  
  bool isTouched = (touchRead(PIN_TOUCH) < 30); 

  // 2. THINK
  // Removed comfortState argument
  brain.update(ears.getAX(), ears.getAY(), ears.getAZ(), isTouched);
  HarvState state = brain.getState();

  // 3. ACT
  eye.update(state);
  face.update(state);
  
  delay(16); 
}