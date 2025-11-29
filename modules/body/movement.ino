#include <Servo.h>

Servo leftServo;
Servo rightServo;

void setup() {
  leftServo.attach(12);
  rightServo.attach(13);
}

void loop() {
  // Placeholder gait: gentle sway
  leftServo.write(90 + 10);
  rightServo.write(90 - 10);
  delay(500);
  leftServo.write(90 - 10);
  rightServo.write(90 + 10);
  delay(500);
}
