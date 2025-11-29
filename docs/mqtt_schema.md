# MQTT Schema

## Topics
- `harv/body/sensors` — Sensor payloads from body modules (JSON: touch, imu_variance, light_level, voice_intensity, attention).
- `harv/body/action/idle` — Idling or heartbeat commands for body modules.
- `harv/body/action/move` — Movement directives (JSON: gait, velocity, heading).
- `harv/body/action/leds` — LED expressions (JSON: pattern, color, duration_ms).
- `harv/bridge/uart/in` — UART messages entering the Pi via bridge.
- `harv/bridge/uart/out` — Messages from Pi to bridge for UART forwarding.
- `harv/camera/stream` — Camera telemetry or compressed frames.
- `harv/logs` — Diagnostic logs.

## Message Examples
```json
{
  "behavior": "explore",
  "state": {"energy": 0.7, "mood": 0.6, "curiosity": 0.8, "trust": 0.5, "fear": 0.2, "engagement": 0.7}
}
```

```json
{
  "type": "sensor",
  "data": {"touch": 1, "imu_variance": 0.3, "light_level": 0.4, "voice_intensity": 0.8, "attention": 0.2}
}
```
