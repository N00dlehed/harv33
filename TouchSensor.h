#ifndef TOUCHSENSOR_H
#define TOUCHSENSOR_H

#include <Arduino.h>

class TouchSensor {
  public:
    TouchSensor(int pin, int threshold = 40);
    void begin();
    bool isTouched();
    int readRaw();

  private:
    int _pin;
    int _threshold;
};

#endif