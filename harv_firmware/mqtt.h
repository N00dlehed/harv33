#pragma once
#include <WiFi.h>
#include <PubSubClient.h>
#include "psyche.h"
#include "led.h"

#define WIFI_SSID     "Is This The Krusty Crab"
#define WIFI_PASSWORD "pinkdogs123"
#define MQTT_SERVER   "10.0.0.95"
#define MQTT_PORT     1883

WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);

void applyMood(const String& mood);

void onMessage(char* topic, byte* payload, unsigned int length) {
  String t = String(topic);
  String msg = "";
  for (int i = 0; i < length; i++) msg += (char)payload[i];

  if (t == "harv/mood") {
    applyMood(msg);
  }
  if (t == "harv/drift") {
    if (msg.indexOf("bright") > 0)       applyMood("bright");
    else if (msg.indexOf("dim") > 0)     applyMood("dim");
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
}

namespace MQTT {
  void connect() {
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) delay(500);
    mqtt.setServer(MQTT_SERVER, MQTT_PORT);
    mqtt.setCallback(onMessage);
    while (!mqtt.connected()) {
      mqtt.connect("harv-body");
      delay(500);
    }
    mqtt.subscribe("harv/mood");
    mqtt.subscribe("harv/psyche");
    mqtt.subscribe("harv/drift");
    mqtt.subscribe("harv/event");
    mqtt.subscribe("harv/wakeup");
  }

  void publish(const char* topic, const char* payload) {
    if (!mqtt.connected()) connect();
    mqtt.publish(topic, payload);
  }

  void loop() {
    if (!mqtt.connected()) connect();
    mqtt.loop();
  }
}