#include "MotionSensor.h"

MotionSensor::MotionSensor() {
  _ax = 0; _ay = 0; _az = 0;
}

void MotionSensor::begin() {
  Wire.begin(); // Join I2C bus
  if (!_mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip");
    // In a real product, we might blink an error LED here
  }
  
  // Set ranges for responsiveness
  _mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  _mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  _mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
}

void MotionSensor::update() {
  sensors_event_t a, g, temp;
  _mpu.getEvent(&a, &g, &temp);

  _ax = a.acceleration.x;
  _ay = a.acceleration.y;
  _az = a.acceleration.z;
}

float MotionSensor::getAX() { return _ax; }
float MotionSensor::getAY() { return _ay; }
float MotionSensor::getAZ() { return _az; }