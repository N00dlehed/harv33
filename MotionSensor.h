#ifndef MOTIONSENSOR_H
#define MOTIONSENSOR_H

#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>

class MotionSensor {
  public:
    MotionSensor();
    void begin();
    void update();
    
    float getAX();
    float getAY();
    float getAZ();

  private:
    Adafruit_MPU6050 _mpu;
    float _ax, _ay, _az;
};

#endif