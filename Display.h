#ifndef DISPLAY_H
#define DISPLAY_H

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "StateMachine.h"

class Display {
  public:
    Display();
    void begin();
    void showBootScreen();
    void update(HarvState state);

  private:
    Adafruit_SSD1306 _oled;
    
    // Animation State
    unsigned long _lastUpdate;
    bool _isBlinking;
    unsigned long _blinkStart;
    unsigned long _nextBlinkTime;
    int _pupilX;
    int _pupilY;
    unsigned long _lastGazeMove;
    unsigned long _nextGazeInterval;

    // Helpers
    void updatePhysics(HarvState state);
    void drawVectorEye(int x_center, int y_center, int w, int h, float eyelid_top, float eyelid_bottom, int offset_x, int offset_y, bool isLeft);

    // Expressions
    void drawNeutralEyes();
    void drawFocusedEyes();
    void drawHappyEyes();
    void drawPanicEyes();
    void drawSleepingEyes(); 
    void drawDizzyEyes();   
};

#endif