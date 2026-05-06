#pragma once
#include <WiFi.h>
#include <WiFiManager.h>   // tzapu/WiFiManager
#include <ESPmDNS.h>
#include <PubSubClient.h>
#include "psyche.h"
#include "led.h"

#define MQTT_PORT      1883

WiFiClient   wifiClient;
PubSubClient mqtt(wifiClient);
WiFiManager  wifiManager;

void applyMood(const String& mood);

// ── Credential wipe ───────────────────────────────────────────

void wipeAndProvision() {
  WiFi.disconnect(true, true);   // erase NVS credentials
  wifiManager.resetSettings();
  delay(500);
  ESP.restart();
}

// ── Physical reset (hold GPIO0 LOW ≥3 s on boot) ──────────────
// Call once from setup(), before MQTT::connect().

void checkPhysicalReset() {
  Serial.println("[reset] hold touch sensor for 5s to reset WiFi...");
  unsigned long start = millis();
  while (millis() - start < 5000) {
    if (touchRead(T0) > 40) return;  // released early
    delay(50);
  }
  Serial.println("[reset] wiping WiFi credentials");
  wipeAndProvision();
}

// ── mDNS resolution ───────────────────────────────────────────

IPAddress _mqttServerIP;

bool resolveMqttServer() {
  for (int i = 0; i < 10; i++) {
    if (WiFi.hostByName("harv.local", _mqttServerIP)) return true;
    delay(300);
  }
  return false;
}

// ── MQTT message handler ──────────────────────────────────────

void onMessage(char* topic, byte* payload, unsigned int length) {
  String t   = String(topic);
  String msg = "";
  for (unsigned int i = 0; i < length; i++) msg += (char)payload[i];

  if (t == "harv/mood") {
    applyMood(msg);
  }
  if (t == "harv/drift") {
    if      (msg.indexOf("bright")  > 0) applyMood("bright");
    else if (msg.indexOf("dim")     > 0) applyMood("dim");
    else if (msg.indexOf("anxious") > 0) applyMood("anxious");
  }
  if (t == "harv/psyche") {
    debugBar("psyche updated");
  }
  if (t == "harv/event") {
    if (msg == "homecoming") {
      applyMood("excited");
      delay(200);
      applyMood("happy");
    }
  }
  if (t == "harv/wakeup") {
    handleWakeup(msg);
  }
  if (t == "harv/cmd") {
    if (msg == "reset_wifi") wipeAndProvision();
  }
}

// ── MQTT namespace ────────────────────────────────────────────

namespace MQTT {

  void _subscribeMqtt() {
    mqtt.subscribe("harv/mood");
    mqtt.subscribe("harv/psyche");
    mqtt.subscribe("harv/drift");
    mqtt.subscribe("harv/event");
    mqtt.subscribe("harv/wakeup");
    mqtt.subscribe("harv/cmd");
  }

  // Reconnect MQTT only — assumes WiFi is already up.
  void _connectMqtt() {
    if (_mqttServerIP == IPAddress(0, 0, 0, 0)) {
      if (!resolveMqttServer()) return;   // broker unreachable
      mqtt.setServer(_mqttServerIP, MQTT_PORT);
      mqtt.setCallback(onMessage);
    }
    int attempts = 0;
    while (!mqtt.connected() && attempts++ < 5) {
      mqtt.connect("harv-body");
      delay(500);
    }
    if (mqtt.connected()) _subscribeMqtt();
  }

  // Full init: captive portal provisioning then MQTT.
  // Call once from setup() (after checkPhysicalReset()).
  void connect() {
    wifiManager.setConfigPortalTimeout(300);   // 5-min portal timeout
    if (!wifiManager.autoConnect("Harv-Setup")) {
      // Timed out without credentials — restart and try again.
      ESP.restart();
    }
    _connectMqtt();
  }

  void publish(const char* topic, const char* payload) {
    if (!mqtt.connected()) _connectMqtt();
    mqtt.publish(topic, payload);
  }

  void loop() {
    if (WiFi.status() != WL_CONNECTED) {
      // WiFi dropped — re-provision.
      connect();
      return;
    }
    if (!mqtt.connected()) _connectMqtt();
    mqtt.loop();
  }

}
