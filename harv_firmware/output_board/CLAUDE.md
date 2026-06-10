# Output Board

Secondary ESP32-WROOM display board. Subscribes to `harv/express/#` and renders text on an OLED. No buzzer, no touch, no IMU.

---

## Pin Assignments

| Function | GPIO |
|---|---|
| OLED SDA | 21 |
| OLED SCL | 22 |
| LED (NeoPixel or plain) | 16 |

OLED: SSD1306 0.96" I2C, address `0x3C`.

---

## Networking

| Setting | Value |
|---|---|
| WiFi SSID | `Is This The Krusty Crab` |
| WiFi password | `secrets.h` → `WIFI_PASSWORD` |
| MQTT broker | `10.0.0.95:1883` |
| MQTT client ID | `harv-output` |
| OTA hostname | `harv-output` |
| OTA password | `harv123` |

---

## MQTT Topic Schema

### Subscribed: `harv/express/#`

All sub-topics use the same JSON payload:

```json
{ "text": "hello", "emotion": "curious" }
```

| Key | Type | Effect |
|---|---|---|
| `text` | string | Displayed on OLED; LED flashes for 500 ms |
| `emotion` | string | Received but currently unused |

Only `text` drives output. An empty or missing `text` key is a no-op.

---

## Secrets

Copy `secrets.h.example` to `secrets.h` and fill in the WiFi password. `secrets.h` is gitignored.

---

## Key Behaviours

- **Boot:** OLED shows `harv.` centred at text size 2.
- **Message received:** OLED updates to `text` value (size 1, word-wrap on); LED turns on for 500 ms.
- **WiFi or MQTT down:** OLED shows `...`; reconnect is attempted every 5 s. On success, OLED returns to `harv.`.
- **OTA:** Always handled first in `loop()`. Flash via `arduino-cli upload --protocol network --port 10.0.0.XX`.
