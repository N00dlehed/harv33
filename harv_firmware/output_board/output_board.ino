#include <WiFi.h>
#include <ArduinoOTA.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "secrets.h"

#define SCREEN_WIDTH        128
#define SCREEN_HEIGHT        64
#define OLED_RESET           -1
#define OLED_ADDR          0x3C
#define LED_PIN              16
#define LED_DURATION        500UL
#define RECONNECT_INTERVAL 5000UL
#define WIFI_TIMEOUT_MS   15000UL

static const char* WIFI_SSID = "Is This The Krusty Crab";
static const char* MQTT_HOST = "10.0.0.95";
static const int   MQTT_PORT = 1883;
static const char* MQTT_SUB  = "harv/express/#";

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
WiFiClient   net;
PubSubClient mqtt(net);

static unsigned long ledOffAt      = 0;
static unsigned long nextReconnect = 0;

// ── Display ────────────────────────────────────────────────────

void oledCentered(const char* text, uint8_t size = 2) {
  display.clearDisplay();
  display.setTextSize(size);
  display.setTextColor(SSD1306_WHITE);
  int16_t  x1, y1;
  uint16_t w,  h;
  display.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
  display.setCursor((SCREEN_WIDTH  - w) / 2,
                    (SCREEN_HEIGHT - h) / 2);
  display.print(text);
  display.display();
}

void oledMessage(const char* text) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setTextWrap(true);
  display.setCursor(0, 0);
  display.print(text);
  display.display();
}

// ── LED ────────────────────────────────────────────────────────

void ledFlash() {
  digitalWrite(LED_PIN, HIGH);
  ledOffAt = millis() + LED_DURATION;
}

// ── MQTT callback ──────────────────────────────────────────────

void onMessage(char* topic, byte* payload, unsigned int length) {
  StaticJsonDocument<256> doc;
  if (deserializeJson(doc, payload, length) != DeserializationError::Ok) return;
  const char* text = doc["text"];
  if (text && *text) {
    oledMessage(text);
    ledFlash();
  }
}

// ── Connectivity ───────────────────────────────────────────────

bool connectWiFi() {
  if (WiFi.status() == WL_CONNECTED) return true;
  oledCentered("...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  unsigned long t = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t < WIFI_TIMEOUT_MS)
    delay(200);
  return WiFi.status() == WL_CONNECTED;
}

bool connectMQTT() {
  if (mqtt.connected()) return true;
  mqtt.connect("harv-output");
  if (mqtt.connected()) {
    mqtt.subscribe(MQTT_SUB);
    return true;
  }
  return false;
}

// Throttled reconnect — called from loop when either link is down.
void ensureConnected() {
  if (millis() < nextReconnect) return;

  if (!connectWiFi()) {
    nextReconnect = millis() + RECONNECT_INTERVAL;
    return;
  }

  if (!connectMQTT()) {
    oledCentered("...");
    nextReconnect = millis() + RECONNECT_INTERVAL;
    return;
  }

  oledCentered("harv.");
  nextReconnect = millis() + RECONNECT_INTERVAL;
}

// ── Setup ──────────────────────────────────────────────────────

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("OLED not found");
    while (true);
  }
  oledCentered("harv.");

  connectWiFi();

  ArduinoOTA.setHostname("harv-output");
  ArduinoOTA.setPassword("harv123");
  ArduinoOTA.begin();

  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  mqtt.setCallback(onMessage);
  mqtt.setBufferSize(512);

  if (connectMQTT()) oledCentered("harv.");
  else               oledCentered("...");
}

// ── Loop ───────────────────────────────────────────────────────

void loop() {
  ArduinoOTA.handle();

  if (WiFi.status() != WL_CONNECTED || !mqtt.connected())
    ensureConnected();

  if (mqtt.connected()) mqtt.loop();

  if (ledOffAt && millis() >= ledOffAt) {
    digitalWrite(LED_PIN, LOW);
    ledOffAt = 0;
  }
}
