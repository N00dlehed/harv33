#include <WiFi.h>
#include <PubSubClient.h>

// Replace with actual credentials
const char* ssid = "YOUR_WIFI";
const char* password = "YOUR_PASSWORD";
const char* mqttServer = "192.168.1.10";
const int mqttPort = 1883;

WiFiClient wifiClient;
PubSubClient client(wifiClient);

HardwareSerial& uart = Serial2;

void setupWifi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  uart.write(payload, length);
  uart.write('\n');
}

void setupMqtt() {
  client.setServer(mqttServer, mqttPort);
  client.setCallback(mqttCallback);
}

void reconnect() {
  while (!client.connected()) {
    if (client.connect("harv-bridge")) {
      client.subscribe("harv/bridge/uart/out");
    } else {
      delay(1000);
    }
  }
}

void setup() {
  uart.begin(115200);
  setupWifi();
  setupMqtt();
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  while (uart.available()) {
    String message = uart.readStringUntil('\n');
    client.publish("harv/bridge/uart/in", message.c_str());
  }
}
