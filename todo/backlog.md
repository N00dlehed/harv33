# HARV33 To-Do Backlog

- [ ] Fill out psyche state JSONs (`psyche/state/*.json`) with baseline identity, traits, temperament, bonding, drift, growth, and capabilities so napgrades have real data to work from.
- [ ] Tighten MQTT client robustness: add reconnect/backoff logic, resubscribe handlers, and error logging in `brain/mqtt/client.py`.
- [ ] Align behavior topics with schema: ensure `BehaviorEngine.behavior_map` uses topics from `brain/mqtt/topics.py` and add a concrete mapping for `rest`; avoid hard-coded `body/leds/wave` unless defined in MQTT schema.
- [ ] Improve sensor/state coherence: reconcile sensor keys in `docs/mqtt_schema.md` with `BrainState.update_from_sensors` and `EmotionEngine.process_triggers` (currently expect `load`, `attention`, `threat`, `touch`, `imu_variance`, `voice_intensity`, `light_level`).
- [ ] Parameterize napgrade schedule via config (use `napgrade_hour` from `brain/config.yaml`) and persist napgrade outputs into memory/RAG.
- [ ] Expand memory system: separate rolling buffer vs. long-term store, add timestamps/tags, and guard against JSON corruption.
- [ ] Replace RAG stubs with real embeddings/index (or a clearer mock contract) and add tests for retrieval.
- [ ] Add behavior execution tests/mocks for MQTT publishing and a basic simulation harness for cognition loop.
- [ ] Secure hardware modules: move WiFi/MQTT credentials to secrets, add reconnection, and define message framing for bridge UART.
- [ ] Camera module: replace placeholder frame publishing with real capture/compression and topic alignment per schema.
