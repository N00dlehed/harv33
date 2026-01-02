#include "StateMachine.h"

StateMachine::StateMachine() {
  _currentState = STATE_IDLE;
  _lastStateChange = 0;
}

// FIXED: Signature now matches header (4 arguments)
void StateMachine::update(float ax, float ay, float az, bool isTouched) {
  float force = max(abs(ax), abs(ay)); 
  
  // 1. SLEEP CHECK (Orientation)
  // Sleep only if FACE DOWN (Z < -6.0)
  bool isFaceDown = (az < -6.0 && abs(ax) < 5.0 && abs(ay) < 5.0 && force < THRESH_MOVE);
  
  // 2. SHAKE CHECK (High Priority)
  if (force > THRESH_SHAKE) {
    _currentState = STATE_DIZZY;
    _lastStateChange = millis();
    return;
  }

  // 3. PRIORITY OVERRIDES (Dizzy persistence)
  if (_currentState == STATE_DIZZY) {
    // Stay dizzy for at least 2 seconds
    if (millis() - _lastStateChange < 2000) return;
  }

  // 4. ACTIVE STATES (Touch / Tilt / Move)
  if (isTouched && _currentState != STATE_TILTED) {
    _currentState = STATE_TOUCHED;
    _lastStateChange = millis();
    return;
  }
  
  if (force > THRESH_TILT) {
    _currentState = STATE_TILTED;
    return;
  }
  
  if (force > THRESH_MOVE) {
    _currentState = STATE_MOVING;
    _lastStateChange = millis();
    return;
  }

  // 5. PASSIVE STATES (Sleep / Idle)
  
  // Only sleep if placed FACE DOWN for 2 seconds
  if (isFaceDown) {
     if (millis() - _lastStateChange > 2000) {
        _currentState = STATE_SLEEPING;
     }
  } 
  else {
     // If we were sleeping, wake up immediately
     if (_currentState == STATE_SLEEPING) {
        _currentState = STATE_IDLE;
     }
     
     // Default to IDLE
     _currentState = STATE_IDLE;
  }
}

HarvState StateMachine::getState() {
  return _currentState;
}