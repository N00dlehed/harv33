# harv33
# Harv33 — Early Architecture & Notes
*A personal project exploring identity-driven behavior in a small embodied system.*

Harv33 is my experiment in building a robot whose **identity** stays consistent even as the underlying AI changes. This repo is a working notebook where I document the architecture, behavior flow, personality rules, sentiment interpretation, and local reflex systems that make Harv feel like Harv — even when his “brain” gets swapped out.

I’ve had a simpler version of Harv running before. Napgrades worked, sentiment handling worked, and Harv could interact cleanly with his cognition layer. I lost that version, so this repo exists to rebuild everything **properly and documented**.

A lot of this is scaffolding based on what I could salvage of my last working version. 
---

## Core Ideas (in plain English)

### Harv’s body and Harv’s “brain” are separate
The hardware/embedded layer handles:
- reflexes  
- gestures  
- LEDs  
- basic expression  
- safety  
- local ML (Edge Impulse)  

The external AI handles:
- meaning  
- high-level reasoning  
- sentiment detection  

But the AI does **not** define who Harv *is*.

### Identity lives outside the AI
Harv has a character sheet:  
a set of traits, limits, emotional boundaries, and consistency rules.

The AI can change, but Harv's personality doesn’t.

### Harv interprets sentiment through his personality
The AI provides:
- sentiment  
- intent  
- emotion guesses  

Harv’s character sheet decides:
- what he *feels* about those emotions  
- how strong the feeling is  
- how it affects his mood  
- how he responds  
- which behaviors are allowed or off-limits  

Sentiment is an input.  
Identity is the filter.  
Behavior is the output.

### Napgrades
“Napgrades” = swapping or improving the AI model without rewriting the system.

Harv goes to sleep → wakes up smarter → still Harv.

### Failure shouldn’t break the vibe
If the network dies or cognition is unavailable, Harv falls back to:
- reflexive behaviors  
- local animations  
- safe emotional presets  

He shouldn’t freeze or act randomly.

---

## Harv’s Architecture (Three Layers)

### 1. Local Reflex Layer (ESP32 + Edge Impulse)
- instant reactions  
- gestures  
- tilt/touch/sound  
- fallback emotion  
- safety rules  

Always available, even offline.

### 2. Identity Layer (Character Sheet)
- personality  
- emotional boundaries  
- behavior constraints  
- sentiment interpretation  
- self-consistency  

Defines who Harv *is*.

### 3. Cognitive Layer (External AI)
- meaning extraction  
- sentiment analysis  
- high-level reasoning  
- suggestion generation  

Completely replaceable.  
Not authoritative over behavior.

---

## AI-Assisted Coding Disclosure

I use AI tools (mostly Codex) while building Harv33.

They help me:
- work faster  
- learn to fill in skill gaps 
- generate boilerplate  
- clean up code  
- document ideas as I go  

All architecture decisions, identity design, and behavior logic are mine.  
Codex is just a tool I use along the way.

---

## On Versioning & Why This Repo Matters

I previously had Harv interacting, reacting, and handling napgrades in a simpler version, but I never properly saved it. Losing that code pushed me to build this repo and document everything.

This is a working notebook as much as a codebase.

---

## Current Status

Harv has worked before.  
This version is about rebuilding the architecture correctly and documenting the mental model.

Some parts are implemented; most are scaffolding.

---

## What This Code Actually Does Today

- **Brain loop (`brain/app.py`):** loads config, connects MQTT, decays/blends emotional state, selects a behavior (`rest/greet/explore/idle`) and publishes MQTT action payloads on a fixed interval while logging recent memories.
- **State & emotions (`brain/core`):** caps and decays emotion variables, maps sensor inputs (touch/imu/voice/light placeholders) to trust/fear/curiosity/engagement, and blends into mood.
- **Behavior engine:** simple heuristic using state + recent memories to choose a behavior; publishes to topics listed in `brain/mqtt/topics.py`.
- **Memory:** rolling JSON buffer saved to `data/memories.json`, with simple recall and keyword lookup stubs.
- **Napgrade stub:** scheduled daily cycle bumps energy, lowers fear, and runs a dummy retrieval pass; no LLM calls yet.
- **RAG stubs:** fake embeddings and in-memory vector index placeholders for later replacement.
- **Hardware sketches (`modules/`):** ESP32 bridge relays UART<->MQTT; body modules drive LEDs/servos and print placeholder sensor values; camera sketch publishes dummy frames.
- **Psyche state (`psyche/state`):** filled JSON baselines for identity, traits, temperament, drift, bonding, growth, and capabilities to feed future napgrade prompts and behavior tuning.


