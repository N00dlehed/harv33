#include <Wire.h>

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);
  delay(1000);
  Serial.println("I2C scanner — SDA=21 SCL=22");

  int found = 0;
  for (byte addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      Serial.printf("  found: 0x%02X\n", addr);
      found++;
    }
  }
  Serial.printf("done — %d device(s)\n", found);
}

void loop() {
  delay(4000);
  Serial.println("--- scan ---");
  int found = 0;
  for (byte addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      Serial.printf("  found: 0x%02X\n", addr);
      found++;
    }
  }
  Serial.printf("done — %d device(s)\n", found);
}
