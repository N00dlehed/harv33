#pragma once
#include <ArduinoIoTCloud.h>
#include <Arduino_ConnectionHandler.h>
#include "psyche.h"
#include "cloud_secrets.h"  // gitignored — copy cloud_secrets.h.example to create

// ── Cloud variables ───────────────────────────────────────────
String cloudMood;           // READWRITE — calls applyMood on change
bool   cloudNapgrade  = false; // READWRITE — publishes run_napgrade to harv/cmd
bool   cloudResetWifi = false; // READWRITE — wipes credentials and re-provisions
String cloudStatus;         // READ — mood + psyche summary, refreshed every 30s

// ── Forward declarations (defined in harv_firmware.ino / mqtt.h) ──
void applyMood(const String& mood);
void wipeAndProvision();
extern String     currentMood;
extern WiFiManager wifiManager;
namespace MQTT { void publish(const char* topic, const char* payload); }

// ── Callbacks ─────────────────────────────────────────────────

void onCloudMoodChange() {
  if (cloudMood.length() > 0) {
    Serial.println("[cloud] mood → " + cloudMood);
    applyMood(cloudMood);
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

// ── Cloud namespace ───────────────────────────────────────────

namespace Cloud {

  WiFiConnectionHandler* _handler  = nullptr;
  unsigned long          _lastStatus = 0;

  void begin() {
    // Borrow credentials WiFiManager already saved — WiFi is already up
    _handler = new WiFiConnectionHandler(
      wifiManager.getWiFiSSID().c_str(),
      wifiManager.getWiFiPass().c_str()
    );

    ArduinoCloud.setDeviceId(DEVICE_ID);
    ArduinoCloud.setSecretDeviceKey(CLOUD_DEVICE_KEY);
    ArduinoCloud.setThingId(THING_ID);

    ArduinoCloud.addProperty(cloudMood,       READWRITE, ON_CHANGE,   onCloudMoodChange);
    ArduinoCloud.addProperty(cloudNapgrade,   READWRITE, ON_CHANGE,   onCloudNapgradeChange);
    ArduinoCloud.addProperty(cloudResetWifi,  READWRITE, ON_CHANGE,   onCloudResetWifiChange);
    ArduinoCloud.addProperty(cloudStatus,     READ,      30 * SECONDS);

    ArduinoCloud.begin(*_handler);
    Serial.println("[cloud] Arduino IoT Cloud initialized");
  }

  void update() {
    ArduinoCloud.update();

    unsigned long now = millis();
    if (now - _lastStatus >= 30000) {
      cloudStatus = currentMood + " | " + Psyche::summary();
      _lastStatus = now;
    }
  }

}
