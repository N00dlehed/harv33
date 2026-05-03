#pragma once
#include <ArduinoJson.h>

namespace Psyche {
  float energy     = 0.55;
  float warmth     = 0.68;
  float curiosity  = 0.72;
  float confidence = 0.48;
  float maturity   = 0.20;
  String baseline  = "idle";
  int   touchCount = 0;

  void onTouch() {
    touchCount++;
    warmth     = min(1.0f, warmth     + 0.02f);
    energy     = min(1.0f, energy     + 0.01f);
    confidence = min(1.0f, confidence + 0.005f);
  }

  void decay() {
    energy = max(0.1f, energy - 0.01f);
  }

  float currentWarmth() { return warmth; }

  bool isSleepy() { return energy < 0.25f; }

  String summary() {
    return "e:" + String(energy, 1) +
          " w:" + String(warmth, 1) +
          " c:" + String(confidence, 1);
  }
}

void handleWakeup(String payload) {
  StaticJsonDocument<256> doc;
  DeserializationError err = deserializeJson(doc, payload);
  if (err) {
    Serial.println("Wakeup parse failed");
    return;
  }

  Psyche::energy     = doc["energy"]     | Psyche::energy;
  Psyche::warmth     = doc["warmth"]     | Psyche::warmth;
  Psyche::curiosity  = doc["curiosity"]  | Psyche::curiosity;
  Psyche::confidence = doc["confidence"] | Psyche::confidence;
  Psyche::maturity   = doc["maturity"]   | Psyche::maturity;
  Psyche::baseline   = doc["baseline"].as<String>();

  Serial.println("=== WAKEUP RECEIVED ===");
  Serial.printf("Energy:     %.2f\n", Psyche::energy);
  Serial.printf("Warmth:     %.2f\n", Psyche::warmth);
  Serial.printf("Curiosity:  %.2f\n", Psyche::curiosity);
  Serial.printf("Confidence: %.2f\n", Psyche::confidence);
  Serial.printf("Maturity:   %.2f\n", Psyche::maturity);
  Serial.println("Baseline:   " + Psyche::baseline);
}