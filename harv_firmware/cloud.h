#pragma once
#include <ArduinoIoTCloud.h>
#include <Arduino_ConnectionHandler.h>
#include "psyche.h"
#include "cloud_secrets.h"  // gitignored — copy cloud_secrets.h.example to create

// ── Device identity (from cloud_secrets.h) ────────────────────
const char DEVICE_LOGIN_NAME[] = SECRET_DEVICE_ID;
const char DEVICE_KEY[]        = SECRET_DEVICE_KEY;

// ── Forward declarations ───────────────────────────────────────
void applyMood(const String& mood);
void wipeAndProvision();
extern String      currentMood;
extern WiFiManager wifiManager;
namespace MQTT { void publish(const char* topic, const char* payload); }

// ── Cloud variables ───────────────────────────────────────────
String cloudMood;
String cloudStatus;
bool   cloudNapgrade  = false;
bool   cloudResetWifi = false;

// ── Callbacks ─────────────────────────────────────────────────

void onCloudMoodChange() {
  if (cloudMood.length() > 0) {
    Serial.println("[cloud] mood → " + cloudMood);
    applyMood(cloudMood);
  }
}

void onCloudStatusChange() {
  // cloudStatus is written by the device — ignore inbound changes
}

void onCloudNapgradeChange() {
  if (cloudNapgrade) {
    Serial.println("[cloud] napgrade triggered");
    MQTT::publish("harv/cmd", "run_napgrade");
    cloudNapgrade = false;
  }
}

void onCloudResetWifiChange() {
  if (cloudResetWifi) {
    Serial.println("[cloud] wifi reset triggered");
    wipeAndProvision();  // does not return
  }
}

// ── Connection handler (populated after WiFiManager connects) ──
WiFiConnectionHandler* ArduinoIoTPreferredConnection = nullptr;

// ── Init (mirrors Arduino Cloud generated pattern) ────────────
void initProperties() {
  ArduinoCloud.setBoardId(DEVICE_LOGIN_NAME);
  ArduinoCloud.setSecretDeviceKey(DEVICE_KEY);
  ArduinoCloud.addProperty(cloudMood,       READWRITE, ON_CHANGE, onCloudMoodChange);
  ArduinoCloud.addProperty(cloudStatus,     READWRITE, ON_CHANGE, onCloudStatusChange);
  ArduinoCloud.addProperty(cloudNapgrade,   READWRITE, ON_CHANGE, onCloudNapgradeChange);
  ArduinoCloud.addProperty(cloudResetWifi,  READWRITE, ON_CHANGE, onCloudResetWifiChange);
}

// ── Cloud namespace ───────────────────────────────────────────
namespace Cloud {

  unsigned long _lastStatus = 0;

  void begin() {
    // WiFiManager has already connected — borrow its credentials
    // so ArduinoCloud can reconnect if WiFi drops
    ArduinoIoTPreferredConnection = new WiFiConnectionHandler(
      wifiManager.getWiFiSSID().c_str(),
      wifiManager.getWiFiPass().c_str()
    );
    initProperties();
    ArduinoCloud.begin(*ArduinoIoTPreferredConnection);
    Serial.println("[cloud] Arduino IoT Cloud initialized");
  }

  void update() {
    ArduinoCloud.update();

    // Push mood + psyche summary to cloudStatus every 30s
    unsigned long now = millis();
    if (now - _lastStatus >= 30000) {
      cloudStatus = currentMood + " | " + Psyche::summary();
      _lastStatus = now;
    }
  }

}
