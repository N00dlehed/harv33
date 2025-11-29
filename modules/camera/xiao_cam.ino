#include <WiFi.h>
#include <PubSubClient.h>

const char* ssid = "YOUR_WIFI";
const char* password = "YOUR_PASSWORD";
const char* mqttServer = "192.168.1.10";
const int mqttPort = 1883;

WiFiClient wifiClient;
PubSubClient client(wifiClient);

void setupWifi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
}

void setupMqtt() {
  client.setServer(mqttServer, mqttPort);
}

void reconnect() {
  while (!client.connected()) {
    if (client.connect("harv-camera")) {
      // Connected
    } else {
      delay(1000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  setupWifi();
  setupMqtt();
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Placeholder camera data
  const char* frame = "frame-bytes";
  client.publish("harv/camera/stream", frame);
  delay(2000);
}
