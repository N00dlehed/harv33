#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ArduinoOTA.h>

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
#include "buzzer.h"
#include "cloud.h"

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
RoboEyes<Adafruit_SSD1306> roboEyes(display);

unsigned long lastIdle  = 0;
unsigned long lastDecay = 0;

#define DECAY_INTERVAL 5000

String currentMood = "idle";

// ── Non-blocking scheduler ────────────────────────────────────
// Replaces all delay() calls in applyMood / doIdleBehavior / loop.

typedef void (*Callback)();

struct Pending { unsigned long at; Callback fn; bool used; };
static Pending _pending[12];

void _schedule(unsigned long msFromNow, Callback fn) {
  for (auto& s : _pending)
    if (!s.used) { s = { millis() + msFromNow, fn, true }; return; }
}

void _runPending() {
  unsigned long now = millis();
  for (auto& s : _pending)
    if (s.used && now >= s.at) { s.used = false; s.fn(); }
}

// ── Named face/LED callbacks (no captures needed) ─────────────
static void _faceDefault()  { roboEyes.setMood(DEFAULT); }
static void _faceHappy()    { roboEyes.setMood(HAPPY); }
static void _faceTired()    { roboEyes.setMood(TIRED); }
static void _faceAngry()    { roboEyes.setMood(ANGRY); }
static void _facePos0()     { roboEyes.setPosition(0); }
static void _facePos3()     { roboEyes.setPosition(3); }
static void _facePos7()     { roboEyes.setPosition(7); }
static void _faceOpen()     { roboEyes.open(); }
static void _restoreLED()   { LED::setState(currentMood); }
static void _restoreFace()  { roboEyes.setMood(DEFAULT); LED::setState(currentMood); }

// ── Debug ─────────────────────────────────────────────────────

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

// ── Mood application (non-blocking) ──────────────────────────

void applyMood(const String& mood) {
  currentMood = mood;
  LED::setState(mood);
  debugState("mood: " + mood);

  if (mood == "excited") {
    roboEyes.anim_confused();
    _schedule(200, _faceHappy);
    Buzzer::excited();
  } else if (mood == "happy") {
    roboEyes.setMood(HAPPY);
    roboEyes.anim_laugh();
    Buzzer::happy();
  } else if (mood == "calm") {
    roboEyes.setMood(DEFAULT);
  } else if (mood == "scared") {
    roboEyes.setMood(TIRED);
    _schedule(800, _faceDefault);
    Buzzer::scared();
  } else if (mood == "curious") {
    roboEyes.setPosition(3);
    _schedule(400, _facePos7);
    _schedule(800, _facePos0);
    Buzzer::curious();
  } else if (mood == "alert") {
    roboEyes.anim_confused();
    _schedule(400, _faceDefault);
    Buzzer::alert();
  } else if (mood == "dim") {
    roboEyes.setMood(TIRED);
  } else if (mood == "bright") {
    roboEyes.setMood(HAPPY);
    _schedule(600, _faceDefault);
  } else if (mood == "anxious") {
    roboEyes.setMood(ANGRY);
  }
}

// ── Emotion handler (harv/emotion JSON) ───────────────────────

void handleEmotion(const String& json) {
  StaticJsonDocument<128> doc;
  if (deserializeJson(doc, json) != DeserializationError::Ok) return;

  if (doc.containsKey("led")) applyMood(doc["led"].as<String>());

  if (doc.containsKey("face")) {
    String face = doc["face"].as<String>();
    if      (face == "happy")   roboEyes.setMood(HAPPY);
    else if (face == "angry")   roboEyes.setMood(ANGRY);
    else if (face == "tired")   roboEyes.setMood(TIRED);
    else                        roboEyes.setMood(DEFAULT);
  }
}

// ── Idle behaviour (non-blocking) ────────────────────────────

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
      _schedule(600, _facePos0);
    } else if (r < 70) {
      roboEyes.close();
      _schedule(80, _faceOpen);
    } else {
      roboEyes.setMood(HAPPY);
    }
  } else if (currentMood == "dim") {
    if (r < 20) {
      roboEyes.close();
      _schedule(80, _faceOpen);
    } else {
      roboEyes.setMood(TIRED);
    }
  } else {
    if (r < (int)(Psyche::curiosity * 30)) {
      roboEyes.setPosition(r < 15 ? 7 : 3);
      _schedule(600, _facePos0);
    } else if (r < 50) {
      roboEyes.close();
      _schedule(80, _faceOpen);
    } else if (r < 60 && Psyche::currentWarmth() > 0.6) {
      roboEyes.setMood(HAPPY);
      _schedule(600, _faceDefault);
    } else {
      roboEyes.setMood(DEFAULT);
    }
  }
}

// ── Setup ─────────────────────────────────────────────────────

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
  Buzzer::begin();

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

  ArduinoOTA.setHostname("harv");
  ArduinoOTA.setPassword("harv123");
  ArduinoOTA.begin();
  Serial.println("OTA ready");

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
  Buzzer::boot();
}

// ── Loop ─────────────────────────────────────────────────────

void loop() {
  ArduinoOTA.handle();   // first — keeps OTA window open every iteration
  _runPending();         // fire any scheduled face/LED callbacks

  roboEyes.update();
  LED::update();
  Buzzer::update();
  MQTT::loop();
  Cloud::update();

  unsigned long now = millis();

  if (Touch::poll()) {
    Psyche::onTouch();
    debugState("touch");
    LED::setState("happy");
    Buzzer::chirp();
    MQTT::publish("harv/touch", "1");
    if (Psyche::currentWarmth() > 0.65) {
      roboEyes.anim_confused();
      _schedule(100, _faceHappy);
    } else {
      roboEyes.setMood(HAPPY);
    }
    _schedule(500, _restoreFace);
  }

  bool lifted, shaken, tapped;
  IMU::poll(lifted, shaken, tapped);

  if (lifted) {
    debugState("lifted");
    roboEyes.anim_confused();
    LED::setState("curious");
    MQTT::publish("harv/motion", "lifted");
    _schedule(500, _restoreFace);
  }

  if (shaken) {
    debugState("shaken");
    roboEyes.setMood(ANGRY);
    LED::setState("scared");
    MQTT::publish("harv/motion", "shaken");
    _schedule(800, _restoreFace);
  }

  if (tapped) {
    debugState("tapped");
    roboEyes.close();
    Buzzer::tap();
    MQTT::publish("harv/motion", "tapped");
    _schedule(80,  _faceOpen);
    _schedule(300, _restoreLED);
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
