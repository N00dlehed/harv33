#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

#include <Arduino.h>

enum HarvState {
  STATE_IDLE,    // Sitting upright
  STATE_MOVING,  // Gentle movement
  STATE_TILTED,  // Leaning too far
  STATE_TOUCHED, // Being petted
  STATE_SLEEPING,// Lying on back
  STATE_DIZZY    // Shaken hard
};

class StateMachine {
  public:
    StateMachine(); 
    // Updated update signature to remove comfortState
    void update(float ax, float ay, float az, bool isTouched);
    HarvState getState();

  private:
    HarvState _currentState;
    unsigned long _lastStateChange;
    
    // Thresholds
    const float THRESH_MOVE = 1.2; 
    const float THRESH_TILT = 5.0; 
    const float THRESH_SHAKE = 18.0; 
};

#endif