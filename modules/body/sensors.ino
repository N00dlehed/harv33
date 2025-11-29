#include <Wire.h>

void setup() {
  Wire.begin();
  Serial.begin(115200);
}

void loop() {
  // Placeholder IMU and touch values
  float imuVariance = 0.1;
  int touch = 0;
  float lightLevel = 0.5;

  Serial.print("IMU:");
  Serial.print(imuVariance);
  Serial.print(",Touch:");
  Serial.print(touch);
  Serial.print(",Light:");
  Serial.println(lightLevel);
  delay(1000);
}
