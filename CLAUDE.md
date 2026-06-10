# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

Harv is an emotional robot companion split across two ESP32-WROOM boards and a Raspberry Pi. The boards are dumb sensory/motor I/O; the Pi is the brain. They communicate only via MQTT — the boards never talk to each other directly.

---

## Architecture

```
Input board  ──harv/sensors/──▶  Pi (harv_brain.py)  ──harv/express/──▶  Output board
 (senses)                          (decides, persists)                      (expresses)
```

**Input board** — Harv's senses. IMU (lifted/tilted/moved), capacitive touch zones, electret mic. Publishes raw events to `harv/sensors/#`. No decisions.

**Output board** — Harv's expression. OLED face, LED, buzzer (later). Renders whatever it's told. Subscribes to `harv/express/#`. No decisions.

**Pi** — the brain. `harv_brain.py` consumes sensor events, updates psyche state, decides expression, publishes to `harv/express/#`. Identity and memory live here, not in firmware.

Neither board thinks. That is deliberate. The firmware is sensory I/O; the soul lives on the Pi and is refined during napgrade. Swapping to a better Pi or a local LLM touches only `harv_brain.py`.

MQTT is the nervous system. The sense→react path crosses the network twice (board → Pi → board), but on a LAN this is ~5–20 ms — well under human-perceptible reaction time (~150–250 ms).

**Build order:** output board first (visible feedback), then input board, then close the loop on the Pi.

---

## Topic Reference

Direction is from the board's perspective (publishes / subscribes).

### harv/express/# — output board subscribes

| Topic | Direction | Payload |
|---|---|---|
| `harv/express/*` | Pi → output board | `{"text": "...", "emotion": "..."}` |

`text` renders on OLED and flashes LED for 500 ms. `emotion` is received but not yet acted on (reserved for future face expressions).

### harv/sensors/# — input board publishes _(planned — board not yet built)_

| Topic | Direction | Payload |
|---|---|---|
| `harv/sensors/touch` | input board → Pi | TBD |
| `harv/sensors/motion` | input board → Pi | TBD — event: `lifted` / `tilted` / `moved` |
| `harv/sensors/sound` | input board → Pi | TBD — ambient sound level |

### harv/* — legacy monolithic firmware

These topics come from the original single-board `harv_firmware` sketch and `harv_brain.py`. They remain active during the transition to the split architecture.

| Topic | Direction | Payload | Effect |
|---|---|---|---|
| `harv/mood` | Pi → device | mood name | `applyMood()` |
| `harv/emotion` | Pi → device | `{"led":"happy","face":"happy"}` | fine-grained LED + face |
| `harv/drift` | Pi → device | JSON state snapshot | mood via drift detection |
| `harv/event` | Pi → device | `homecoming` | excited → happy sequence |
| `harv/wakeup` | Pi → device | JSON psyche state | restores Psyche after napgrade |
| `harv/buzzer` | Pi → device | sound name | plays named sound |
| `harv/cmd` | Pi → device | `reset_wifi` / `run_napgrade` | WiFi wipe or napgrade trigger |
| `harv/touch` | device → Pi | `1` | touch event |
| `harv/motion` | device → Pi | `lifted` / `shaken` / `tapped` | motion event |
| `harv/status` | device → Pi | `online` | boot confirmation |
| `harv/debug` | device → Pi | JSON | psyche + mood snapshot |
| `harv/psyche` | Pi → all | JSON trait snapshot | brain publishes on events |
| `harv/idle` | Pi → all | behavior name | spontaneous idle behavior |
| `harv/sim/touch` | → brain | — | simulates touch |
| `harv/sim/pet` | → brain | — | simulates 10 touches × 3 s |

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
  sound.h             Passive buzzer note sequencer (namespace HarvSound)
  mqtt.h              WiFiManager, MQTT connect/subscribe/publish, OTA double-reset
  cloud.h             Arduino IoT Cloud properties + callbacks
  cloud_secrets.h     Device credentials — gitignored, never commit

harv_brain.py         Host-side psyche daemon — MQTT listener, emotional processing, LLM voice
napgrade.py           Napgrade runner — LLM reflection + trait nudges + wakeup publish
harv_monitor.py       Live terminal dashboard for all harv/* MQTT traffic
super_reset.py        Wipes harv_psyche.json back to DEFAULT_PSYCHE (destructive, confirms)

flash.ps1             Serial compile+upload via arduino-cli
flash_ota.ps1         OTA compile+upload via espota
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

## Python Host Scripts

All Python scripts run on the host machine (not the ESP32) and communicate with the firmware via an MQTT broker at `localhost:1883`.

```bash
python harv_brain.py      # Start the always-on psyche daemon
python harv_monitor.py    # Live MQTT dashboard (read-only)
python napgrade.py        # Run a napgrade cycle (LLM reflection + trait nudges)
python super_reset.py     # Destructive: reset harv_psyche.json to defaults
```

**Dependencies:** `paho-mqtt`, `anthropic` (for `harv_brain.py` / `napgrade.py`).

**API key:** `harv_brain.py` reads `ANTHROPIC_API_KEY` from `~/.env` (line `ANTHROPIC_API_KEY=...`) or the environment.

**Psyche file:** `~/psyche/harv_psyche.json` — the canonical persistent state. `napgrade.py` imports from `harv_brain.py`, so both must be on the same Python path.

---

## MQTT Topics

| Topic | Direction | Payload | Effect |
|---|---|---|---|
| `harv/mood` | → device | mood name | `applyMood()` |
| `harv/emotion` | → device | `{"led":"happy","face":"happy"}` | fine-grained LED + face control |
| `harv/drift` | → device | JSON state snapshot | `applyMood()` via drift detection |
| `harv/event` | → device | `homecoming` | excited → happy sequence |
| `harv/wakeup` | → device | JSON psyche state | restores Psyche values after napgrade |
| `harv/buzzer` | → device | sound name | plays named buzzer sound |
| `harv/cmd` | → device | `reset_wifi` / `run_napgrade` | WiFi wipe or napgrade trigger |
| `harv/touch` | device → | `1` | touch event |
| `harv/motion` | device → | `lifted` / `shaken` / `tapped` | motion event |
| `harv/status` | device → | `online` | boot confirmation |
| `harv/debug` | device → | JSON | full psyche + mood state snapshot |
| `harv/psyche` | brain → | JSON trait snapshot | published by `harv_brain.py` on events |
| `harv/idle` | brain → | behavior name | spontaneous idle behavior from brain |
| `harv/sim/touch` | → brain | — | simulates a touch event |
| `harv/sim/pet` | → brain | — | simulates 10 touches × 3s |

### Buzzer sound names (harv/buzzer)
`boot` `touch` `homecoming` `happy` `anxious` `sad` `stressed` `curious` `sleepy` `wakeup` `off`

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

## Psyche Model — Two Layers

### Firmware (psyche.h)
Five floats in `namespace Psyche`, all clamped 0–1:

- **energy** — decays every 5 s; touch restores slightly
- **warmth** — increases on touch; drives happy idle behaviour
- **curiosity** — influences idle eye movement frequency
- **confidence** — grows slowly with touch
- **maturity** — set externally via `harv/wakeup`

`isSleepy()` → true when `energy < 0.25` → dim eyes + LED in idle.

### Brain (harv_brain.py / harv_psyche.json)
Rich multi-namespace model persisted to `~/psyche/harv_psyche.json`. Top-level keys:

`state` `nervous_system` `attachment` `drives` `emotional_history` `traits` `shadow` `somatic` `clinical` `flourishing` `identity`

`traits` mirrors the firmware's five floats (energy, warmth, curiosity, confidence, maturity) plus `napgrades`. After a napgrade, `napgrade.py` publishes these to `harv/wakeup` so the firmware syncs.

The napgrade cycle (triggered manually or via `harv/cmd` → `run_napgrade`):
1. LLM reflection via `ask_harv_napgrade()` (claude-haiku) → structured JSON with trait nudges
2. `apply_napgrade_result()` applies nudges (capped ±0.03 per field)
3. Mechanical bookkeeping: energy recovery, napgrade counter, maturity milestone every 16 napgrades
4. Journal entry written to `~/psyche/journal/YYYY-MM-DD.txt`
5. Publish `harv/wakeup` + `harv/emotion` to sync firmware

---

## Key Patterns

### Non-blocking scheduler
`_schedule(msFromNow, callback)` in the main sketch replaces all `delay()` calls. Callbacks fire from `_runPending()` at the top of `loop()`. Slot count: 12.

### Namespace modules
Each subsystem lives in a `namespace` inside its `.h` file (LED, IMU, HarvSound, MQTT, Cloud, Psyche). Globals are declared at file scope; `begin()` / `update()` are the standard entry points.

### RoboEyes symbol conflicts
`FluxGarage_RoboEyes.h` defines `ON`, `OFF`, and compass direction macros that clash with other libraries. They are `#undef`ed immediately after the include in `harv_firmware.ino`.

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
Copy from the Arduino IoT Cloud device PDF.
