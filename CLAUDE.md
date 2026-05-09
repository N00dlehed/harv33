# Harv — Firmware

Harv is an emotional robot companion built on an ESP32. It has animated OLED eyes, a breathing NeoPixel, a passive piezo buzzer, touch sensing, motion detection (IMU), and connects to MQTT + Arduino IoT Cloud.

---

## Hardware

| Component | Part | Pin(s) |
|---|---|---|
| MCU | uPesy ESP32 Wroom DevKit | — |
| Display | SSD1306 OLED 128×64 | I²C — SDA 21, SCL 22 |
| LED | NeoPixel (1 pixel) | GPIO 16 |
| Touch | Capacitive touch pad | GPIO 4 (T0) |
| IMU | MPU6050 | I²C — SDA 21, SCL 22 |
| Buzzer | Passive piezo | GPIO 25 |

OTA password: `harv123`  
mDNS hostname: `harv.local`  
Static IP (current): `10.0.0.69`

---

## File Structure

```
harv_firmware/
  harv_firmware.ino   Main sketch — setup(), loop(), applyMood(), scheduler
  psyche.h            Emotional state model (energy, warmth, curiosity, confidence, maturity)
  display.h           OLED helpers, debugBar()
  touch.h             Capacitive touch polling
  imu.h               MPU6050 — lifted / shaken / tapped detection
  led.h               NeoPixel breathing + mood colours (namespace LED)
  buzzer.h            Passive buzzer note sequencer (namespace Buzzer)
  mqtt.h              WiFiManager, MQTT connect/subscribe/publish, OTA double-reset
  cloud.h             Arduino IoT Cloud properties + callbacks
  cloud_secrets.h     Device credentials — gitignored, never commit
```

---

## Flashing

### Serial (USB) — required after partition scheme change
```powershell
.\flash.ps1
```
Compiles and uploads via COM3 at 921600 baud.

### OTA (WiFi) — normal workflow
```powershell
.\flash_ota.ps1
```
Compiles, then uploads to `10.0.0.69` via espota. Falls back to `espota.py` directly if arduino-cli port discovery fails.

### Partition scheme
`min_spiffs` — two 1.9 MB OTA slots, minimal SPIFFS. Required because the firmware exceeds 1 MB (default slots are too small) and `huge_app` has only one slot (breaks OTA).

---

## MQTT Topics

| Topic | Direction | Payload | Effect |
|---|---|---|---|
| `harv/mood` | → device | mood name | `applyMood()` |
| `harv/emotion` | → device | `{"led":"happy","face":"happy"}` | fine-grained LED + face control |
| `harv/drift` | → device | string containing `bright`/`dim`/`anxious` | `applyMood()` |
| `harv/event` | → device | `homecoming` | excited → happy sequence |
| `harv/wakeup` | → device | JSON psyche state | restores Psyche values after napgrade |
| `harv/buzzer` | → device | sound name | plays named buzzer sound |
| `harv/cmd` | → device | `reset_wifi` | wipes WiFi credentials and re-provisions |
| `harv/touch` | device → | `1` | touch event |
| `harv/motion` | device → | `lifted` / `shaken` / `tapped` | motion event |
| `harv/status` | device → | `online` | boot confirmation |
| `harv/debug` | device → | JSON | full psyche + mood state snapshot |

### Buzzer sound names (harv/buzzer)
`boot` `chirp` `happy` `excited` `sad` `scared` `alert` `tap` `curious` `off`

---

## Moods

Moods affect LED colour/breathing, eye expression, and buzzer sound simultaneously.

| Mood | LED | Eyes | Sound |
|---|---|---|---|
| `idle` | soft blue | default | — |
| `happy` | warm amber | HAPPY + laugh | happy jingle |
| `excited` | bright yellow | confused → happy | excited fanfare |
| `calm` | warm orange | default | — |
| `bright` | bright amber | HAPPY → default | — |
| `dim` | deep dim blue | TIRED | — |
| `curious` | cyan | position sweep | curious two-note |
| `scared` | pale blue | TIRED → default | descending tones |
| `anxious` | red-orange | ANGRY | — |
| `alert` | (LED state) | confused → default | double beep |

---

## Psyche Model

Five floats in `namespace Psyche`, all clamped 0–1:

- **energy** — decays every 5 s; touch restores slightly
- **warmth** — increases on touch; drives happy idle behaviour
- **curiosity** — influences idle eye movement frequency
- **confidence** — grows slowly with touch
- **maturity** — set externally via `harv/wakeup`

`isSleepy()` → true when `energy < 0.25` → dim eyes + LED in idle.

Psyche is restored after a "napgrade" via the `harv/wakeup` MQTT topic with a JSON payload of all five values plus `baseline`.

---

## Key Patterns

### Non-blocking scheduler
`_schedule(msFromNow, callback)` in the main sketch replaces all `delay()` calls. Callbacks fire from `_runPending()` at the top of `loop()`. Slot count: 12.

### Namespace modules
Each subsystem lives in a `namespace` inside its `.h` file (LED, IMU, Buzzer, MQTT, Cloud, Psyche). Globals are declared at file scope; `begin()` / `update()` are the standard entry points.

### Forward declarations in mqtt.h
`onMessage()` in `mqtt.h` calls functions defined later in the `.ino`. Add a forward declaration near the top of `mqtt.h` (alongside `applyMood` and `handleEmotion`) rather than reordering files.

### Double-reset WiFi provisioning
Reset twice within 10 s → wipes NVS WiFi credentials → starts captive portal (`Harv-Setup` SSID). Also triggerable via `harv/cmd` → `reset_wifi`.

---

## Secrets

`cloud_secrets.h` is gitignored. It defines:
```cpp
#define SECRET_DEVICE_ID  "..."
#define SECRET_DEVICE_KEY "..."
```
Copy from the Arduino IoT Cloud device PDF. The example template was at `cloud_secrets.h.example` (now removed from repo).
