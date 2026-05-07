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
String cloudMessage;
bool   cloudTouch       = false;
bool   cloudHomecoming  = false;
bool   cloudPetSession  = false;
bool   cloudShake       = false;
bool   cloudLift        = false;
String cloudMood;
bool   cloudNapgrade    = false;
bool   cloudResetWifi   = false;
String cloudStatus;

// ── Callbacks ─────────────────────────────────────────────────

void onCloudMessageChange() {
  if (cloudMessage.length() > 0) {
    Serial.println("[cloud] message → " + cloudMessage);
    MQTT::publish("harv/message", cloudMessage.c_str());
  }
}

void onCloudTouchChange() {
  if (cloudTouch) {
    Serial.println("[cloud] sim touch");
    MQTT::publish("harv/sim/touch", "1");
    cloudTouch = false;
  }
}

void onCloudHomecomingChange() {
  if (cloudHomecoming) {
    Serial.println("[cloud] sim homecoming");
    MQTT::publish("harv/event", "homecoming");
    cloudHomecoming = false;
  }
}

void onCloudPetSessionChange() {
  if (cloudPetSession) {
    Serial.println("[cloud] sim pet session");
    MQTT::publish("harv/sim/pet", "1");
    cloudPetSession = false;
  }
}

void onCloudShakeChange() {
  if (cloudShake) {
    Serial.println("[cloud] sim shake");
    MQTT::publish("harv/motion", "shaken");
    cloudShake = false;
  }
}

void onCloudLiftChange() {
  if (cloudLift) {
    Serial.println("[cloud] sim lift");
    MQTT::publish("harv/motion", "lifted");
    cloudLift = false;
  }
}

void onCloudMoodChange() {
  if (cloudMood.length() > 0) {
    Serial.println("[cloud] mood → " + cloudMood);
    MQTT::publish("harv/mood", cloudMood.c_str());
  }
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

void onCloudStatusChange() {
  // cloudStatus is written by the device — ignore inbound changes
}

// ── Connection handler (populated after WiFiManager connects) ──
WiFiConnectionHandler* ArduinoIoTPreferredConnection = nullptr;

// ── Init (mirrors Arduino Cloud generated pattern) ────────────
void initProperties() {
  ArduinoCloud.setBoardId(DEVICE_LOGIN_NAME);
  ArduinoCloud.setSecretDeviceKey(DEVICE_KEY);
  ArduinoCloud.addProperty(cloudMessage,    READWRITE, ON_CHANGE, onCloudMessageChange);
  ArduinoCloud.addProperty(cloudTouch,      READWRITE, ON_CHANGE, onCloudTouchChange);
  ArduinoCloud.addProperty(cloudHomecoming, READWRITE, ON_CHANGE, onCloudHomecomingChange);
  ArduinoCloud.addProperty(cloudPetSession, READWRITE, ON_CHANGE, onCloudPetSessionChange);
  ArduinoCloud.addProperty(cloudShake,      READWRITE, ON_CHANGE, onCloudShakeChange);
  ArduinoCloud.addProperty(cloudLift,       READWRITE, ON_CHANGE, onCloudLiftChange);
  ArduinoCloud.addProperty(cloudMood,       READWRITE, ON_CHANGE, onCloudMoodChange);
  ArduinoCloud.addProperty(cloudNapgrade,   READWRITE, ON_CHANGE, onCloudNapgradeChange);
  ArduinoCloud.addProperty(cloudResetWifi,  READWRITE, ON_CHANGE, onCloudResetWifiChange);
  ArduinoCloud.addProperty(cloudStatus,     READWRITE, ON_CHANGE, onCloudStatusChange);
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

    // Push mood + psyche summary to cloudStatus every 60s
    unsigned long now = millis();
    if (now - _lastStatus >= 60000) {
      cloudStatus = currentMood + " | " + Psyche::summary();
      _lastStatus = now;
    }
  }

}
