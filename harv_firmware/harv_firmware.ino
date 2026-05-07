#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include <FluxGarage_RoboEyes.h>
#undef ON
#undef OFF
#undef N
#undef NE
#undef E
#undef SE
#undef S
#undef SW
#undef W
#undef NW

#include "psyche.h"
#include "display.h"
#include "touch.h"
#include "mqtt.h"
#include "imu.h"
#include "led.h"
#include "cloud.h"

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
RoboEyes<Adafruit_SSD1306> roboEyes(display);

unsigned long lastIdle  = 0;
unsigned long lastDecay = 0;

#define DECAY_INTERVAL 5000

String currentMood = "idle";

void debugState(const String& reason) {
  String payload = "{";
  payload += "\"reason\":\"" + reason + "\",";
  payload += "\"mood\":\"" + currentMood + "\",";
  payload += "\"energy\":" + String(Psyche::energy, 2) + ",";
  payload += "\"warmth\":" + String(Psyche::warmth, 2) + ",";
  payload += "\"curiosity\":" + String(Psyche::curiosity, 2) + ",";
  payload += "\"confidence\":" + String(Psyche::confidence, 2) + ",";
  payload += "\"maturity\":" + String(Psyche::maturity, 2) + ",";
  payload += "\"baseline\":\"" + Psyche::baseline + "\"";
  payload += "}";
  MQTT::publish("harv/debug", payload.c_str());
  Serial.println(payload);
}

void applyMood(const String& mood) {
  currentMood = mood;
  LED::setState(mood);
  debugState("mood: " + mood);

  if (mood == "excited") {
    roboEyes.anim_confused();
    delay(200);
    roboEyes.setMood(HAPPY);
  } else if (mood == "happy") {
    roboEyes.setMood(HAPPY);
    roboEyes.anim_laugh();
  } else if (mood == "calm") {
    roboEyes.setMood(DEFAULT);
  } else if (mood == "scared") {
    roboEyes.setMood(TIRED);
    delay(800);
    roboEyes.setMood(DEFAULT);
  } else if (mood == "curious") {
    roboEyes.setPosition(3);
    delay(400);
    roboEyes.setPosition(7);
    delay(400);
    roboEyes.setPosition(0);
  } else if (mood == "alert") {
    roboEyes.anim_confused();
    delay(400);
    roboEyes.setMood(DEFAULT);
  } else if (mood == "dim") {
    roboEyes.setMood(TIRED);
  } else if (mood == "bright") {
    roboEyes.setMood(HAPPY);
    delay(600);
    roboEyes.setMood(DEFAULT);
  } else if (mood == "anxious") {
    roboEyes.setMood(ANGRY);
  }
}

void doIdleBehavior() {
  if (Psyche::isSleepy()) {
    debugState("idle: sleepy");
    roboEyes.setMood(TIRED);
    LED::setState("dim");
    return;
  }

  int r = random(100);

  if (currentMood == "bright" || currentMood == "excited") {
    if (r < 40) {
      roboEyes.setPosition(r < 20 ? 7 : 3);
      delay(600);
      roboEyes.setPosition(0);
    } else if (r < 70) {
      roboEyes.close();
      delay(80);
      roboEyes.open();
    } else {
      roboEyes.setMood(HAPPY);
    }
  } else if (currentMood == "dim") {
    if (r < 20) {
      roboEyes.close();
      delay(80);
      roboEyes.open();
    } else {
      roboEyes.setMood(TIRED);
    }
  } else {
    if (r < (int)(Psyche::curiosity * 30)) {
      roboEyes.setPosition(r < 15 ? 7 : 3);
      delay(600);
      roboEyes.setPosition(0);
    } else if (r < 50) {
      roboEyes.close();
      delay(80);
      roboEyes.open();
    } else if (r < 60 && Psyche::currentWarmth() > 0.6) {
      roboEyes.setMood(HAPPY);
      delay(600);
      roboEyes.setMood(DEFAULT);
    } else {
      roboEyes.setMood(DEFAULT);
    }
  }
}

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED not found");
    while (true);
  }

  display.clearDisplay();
  Touch::begin();
  IMU::begin();
  LED::begin();

  LED::setState("dim");

  roboEyes.begin(SCREEN_WIDTH, SCREEN_HEIGHT, 60);
  roboEyes.setAutoblinker(ON, 3, 2);
  roboEyes.setIdleMode(ON, 4, 2);
  roboEyes.setCuriosity(ON);
  roboEyes.close();
  delay(300);
  roboEyes.open();

  MQTT::connect();
  Cloud::begin();

  LED::setState("warm");
  delay(500);
  roboEyes.setMood(HAPPY);
  roboEyes.anim_laugh();
  LED::setState("bright");
  delay(800);
  roboEyes.setMood(DEFAULT);
  LED::setState("idle");

  Serial.println("Harv online");
  Serial.println(Psyche::summary());
  MQTT::publish("harv/status", "online");
}

void loop() {
  roboEyes.update();
  LED::update();
  MQTT::loop();
  Cloud::update();

  unsigned long now = millis();

  if (Touch::poll()) {
    Psyche::onTouch();
    debugState("touch");
    LED::setState("happy");
    MQTT::publish("harv/touch", "1");
    if (Psyche::currentWarmth() > 0.65) {
      roboEyes.anim_confused();
      delay(100);
      roboEyes.setMood(HAPPY);
    } else {
      roboEyes.setMood(HAPPY);
    }
    delay(500);
    roboEyes.setMood(DEFAULT);
    LED::setState(currentMood);
  }

  bool lifted, shaken, tapped;
  IMU::poll(lifted, shaken, tapped);

  if (lifted) {
    debugState("lifted");
    roboEyes.anim_confused();
    LED::setState("curious");
    MQTT::publish("harv/motion", "lifted");
    delay(500);
    roboEyes.setMood(DEFAULT);
    LED::setState(currentMood);
  }

  if (shaken) {
    debugState("shaken");
    roboEyes.setMood(ANGRY);
    LED::setState("scared");
    MQTT::publish("harv/motion", "shaken");
    delay(800);
    roboEyes.setMood(DEFAULT);
    LED::setState(currentMood);
  }

  if (tapped) {
    debugState("tapped");
    roboEyes.close();
    delay(80);
    roboEyes.open();
    MQTT::publish("harv/motion", "tapped");
    delay(300);
  }

  if (now - lastIdle > 2000) {
    doIdleBehavior();
    lastIdle = now;
  }

  if (now - lastDecay > DECAY_INTERVAL) {
    Psyche::decay();
    lastDecay = now;
  }
}