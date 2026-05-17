#ifndef NEOPIXELFX_H
#define NEOPIXELFX_H

#include <Adafruit_NeoPixel.h>
#include "StateMachine.h" 

class NeoPixelFX {
  public:
    NeoPixelFX(int pin, int numPixels);
    void begin();
    void update(HarvState state); 

  private:
    Adafruit_NeoPixel _strip;
    int _numPixels;
    float _breathPhase;
};

#endif