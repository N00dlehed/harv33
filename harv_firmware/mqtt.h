#pragma once
#include <WiFi.h>
#include <WiFiManager.h>          // tzapu/WiFiManager
#include <ESPmDNS.h>
#include <PubSubClient.h>
#define ESP_DRD_USE_EEPROM   true
#define DOUBLERESETDETECTOR_DEBUG false
#include <ESP_DoubleResetDetector.h>  // khoih-prog/ESP_DoubleResetDetector
#include "psyche.h"
#include "led.h"

#define MQTT_PORT     1883
#define DRD_TIMEOUT   10     // seconds — reset twice within this window to trigger
#define DRD_ADDRESS   0      // EEPROM address for DRD flag

WiFiClient          wifiClient;
PubSubClient        mqtt(wifiClient);
WiFiManager         wifiManager;
DoubleResetDetector* drd = nullptr;

void applyMood(const String& mood);

// ── Credential wipe ───────────────────────────────────────────

void wipeAndProvision() {
  WiFi.disconnect(true, true);   // erase NVS credentials
  wifiManager.resetSettings();
  delay(500);
  ESP.restart();
}

// ── Double-reset detection ────────────────────────────────────
// Call once from MQTT::connect() after LED::begin().
// First reset: flashes blue for DRD_TIMEOUT seconds as a hint.
// Second reset within that window: wipes credentials and provisions.

void _drdFlashBlue(unsigned long durationMs) {
  unsigned long start = millis();
  while (millis() - start < durationMs) {
    unsigned long phase = (millis() - start) % 200;
    led.setPixelColor(0, phase < 100 ? led.Color(0, 0, 255) : led.Color(0, 0, 0));
    led.show();
    drd->loop();
    delay(20);
  }
  led.setPixelColor(0, led.Color(0, 0, 0));
  led.show();
}

void checkDoubleReset() {
  drd = new DoubleResetDetector(DRD_TIMEOUT, DRD_ADDRESS);
  if (drd->detectDoubleReset()) {
    Serial.println("[drd] double reset — wiping WiFi credentials");
    wipeAndProvision();  // does not return
  }
  Serial.println("[drd] window open — reset again within 10s to enter setup");
  _drdFlashBlue(DRD_TIMEOUT * 1000UL);
  drd->stop();
  Serial.println("[drd] window closed, continuing boot");
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

  // Full init: double-reset check, captive portal provisioning, then MQTT.
  // Call once from setup() after LED::begin().
  void connect() {
    checkDoubleReset();
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
