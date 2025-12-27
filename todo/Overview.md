# HARV33 — Master TODO Overview

## PHASE 0 — FOUNDATION (Stability First)
- [ ] Create `RUN.md` (how to boot, flash, test)
- [ ] Create `KNOWN_GOOD.md` (working states + commits)
- [ ] Decide single “demo goal” (what Harv must always do)

---

## PHASE 1 — COMMUNICATION SPINE
- [ ] Create `brain/mqtt/topics.py`
- [ ] Define canonical MQTT topics
- [ ] Define single action payload schema
- [ ] Implement brain heartbeat (2s)
- [ ] Implement ESP32 heartbeat (2s)
- [ ] Implement ACK response from ESP32
- [ ] Verify round-trip messaging stability

---

## PHASE 2 — REFLEX LAYER (ESP32)
- [ ] Read touch input
- [ ] Debounce touch sensor
- [ ] Implement reflex behaviors:
  - [ ] Tap → curious blink
  - [ ] Hold → boundary response
  - [ ] Double tap → positive response
- [ ] Add offline fallback mode
- [ ] Confirm reflexes work without brain connected

---

## PHASE 3 — IDENTITY LAYER
- [ ] Create `identity/core.json`
- [ ] Define:
  - traits
  - temperament
  - boundaries
  - forbidden actions
- [ ] Create `identity/filter.py`
- [ ] Enforce identity filtering on all actions
- [ ] Log denied or modified actions

---

## PHASE 4 — EMOTIONAL STATE ENGINE
- [ ] Define emotional variables:
  - energy
  - curiosity
  - trust
  - caution
  - engagement
- [ ] Implement decay logic
- [ ] Map sensor input → emotional deltas
- [ ] Derive mood label from state
- [ ] Expose mood to behavior engine

---

## PHASE 5 — BEHAVIOR ENGINE
- [ ] Define behaviors:
  - rest
  - idle
  - explore
  - greet
  - boundary
- [ ] Create behavior selector
- [ ] Limit actions per behavior (1–3 max)
- [ ] Prevent behavior thrashing
- [ ] Validate behavior → action mapping

---

## PHASE 6 — MEMORY SYSTEM
- [ ] Create rolling memory buffer (last 50 events)
- [ ] Store:
  - sensor events
  - actions
  - mood changes
- [ ] Implement keyword lookup
- [ ] Keep memory lightweight (no embeddings yet)

---

## PHASE 7 — NAPGRADES
- [ ] Snapshot identity + state
- [ ] Define napgrade trigger
- [ ] Run fixed test prompts
- [ ] Compare pre/post behavior
- [ ] Approve or rollback upgrade
- [ ] Log all napgrades

---

## PHASE 8 — HARDENING
- [ ] Validate crash recovery
- [ ] Confirm no dependency on AI availability
- [ ] Confirm reflex-only operation works
- [ ] Document known limits



