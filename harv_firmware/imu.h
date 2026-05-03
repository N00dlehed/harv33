#pragma once
#include <MPU6050.h>

MPU6050 mpu;

namespace IMU {
  float baseZ = 0;
  bool lifted = false;
  bool shaken = false;

  void begin() {
    mpu.initialize();
    // Calibrate baseline
    int16_t ax, ay, az, gx, gy, gz;
    mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
    baseZ = az;
  }

  void poll(bool &wasLifted, bool &wasShaken, bool &wasTapped) {
    wasLifted = false;
    wasShaken = false;
    wasTapped = false;

    int16_t ax, ay, az, gx, gy, gz;
    mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

    // Lifted — Z acceleration drops significantly
    if (az < baseZ - 8000) {
      wasLifted = true;
    }

    // Shaken — high gyro activity
    int totalGyro = abs(gx) + abs(gy) + abs(gz);
    if (totalGyro > 50000) {
      wasShaken = true;
    }

    // Tapped — sharp acceleration spike
    int totalAccel = abs(ax) + abs(ay);
    if (totalAccel > 25000) {
      wasTapped = true;
    }
  }
}