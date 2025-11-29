# HARV-33 Architecture

```
[Pi Brain] --UART--> [ESP32 Bridge] --WiFi/MQTT--> Cloud
    |                                    |
    | MQTT                               | MQTT
    v                                    v
[Body Modules (ESP32)] <--------------> [Optional XIAO S3 Cam]
```

- **Brain (Raspberry Pi):** Runs cognition loop, emotion engine, behavior selection, and napgrade routines.
- **Bridge (ESP32):** Connects Pi to WiFi MQTT, relays UART messages, and mirrors topics.
- **Body Modules (ESP32):** Drive servos, read IMU/touch sensors, and render LEDs, listening to action topics.
- **Camera (XIAO ESP32-S3):** Publishes video snapshots or telemetry to camera topics.
- **MQTT Fabric:** All components exchange events and commands via the shared topic schema.
