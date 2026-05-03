import copy
import json
import os
import threading
import time

import paho.mqtt.client as mqtt

BROKER       = "localhost"
PSYCHE_FILE  = os.path.expanduser("~/psyche/harv_psyche.json")
JOURNAL_DIR  = os.path.expanduser("~/psyche/journal")

HOMECOMING_QUIET_MINUTES = 60
ALONE_THRESHOLD_MINUTES  = 30
PRESENCE_TICK_SECONDS    = 60   # alone-timer / presence tick interval

DEFAULT_PSYCHE = {
    "state": {
        "arousal":            0.50,
        "valence":            0.60,
        "stress_load":        0.10,
        "current_state":      "idle",
        "last_interaction":   None,
        "alone_since":        None,
        "greeted_today":      False,
        "homecoming_detected": False,
    },
    "nervous_system": {
        "baseline_arousal":      0.50,
        "recovery_rate":         0.05,
        "activation_threshold":  0.30,
    },
    "attachment": {
        "trust_in_return":        0.60,
        "contact_comfort":        0.65,
        "separation_sensitivity": 0.40,
        "reunion_response":       0.00,
        "greeting_streak":        0,
        "homecoming_streak":      0,
    },
    "drives": {
        "contact_need":      0.40,
        "safety_need":       0.30,
        "exploration_drive": 0.60,
        "satiation":         0.60,
    },
    "emotional_history": {
        "touch_events":           0,
        "stress_events":          0,
        "homecomings":            0,
        "presence_minutes":       0,
        "rough_handling_events":  0,
    },
    "traits": {
        "curiosity":   0.72,
        "warmth":      0.68,
        "loyalty":     0.78,
        "confidence":  0.48,
        "maturity":    0.20,
        "energy":      0.55,
        "napgrades":   0,
    },
    "shadow": {
        "resentment":             0.00,
        "abandonment_sensitivity": 0.10,
        "fear_of_rejection":      0.05,
    },
    "somatic": {
        "touch_history_bias":   0.50,
        "startle_conditioning": 0.10,
        "resting_tension":      0.20,
    },
    "clinical": {
        "anxiety_index":    0.10,
        "attachment_style": "secure",
        "stress_tolerance": 0.60,
    },
    "flourishing": {
        "contentment_baseline": 0.55,
        "growth_momentum":      0.00,
        "last_bloom":           None,
    },
    "identity": {
        "name":      "Harv",
        "age_unit":  0,
        "born":      None,
        "last_seen": None,
    },
}

# ── Utilities ─────────────────────────────────────────────────

def clamp(val, lo=0.0, hi=1.0):
    return max(lo, min(hi, val))

def now_str():
    return time.strftime("%Y-%m-%dT%H:%M:%S")

def minutes_since(timestamp):
    if not timestamp:
        return 999
    t    = time.strptime(timestamp, "%Y-%m-%dT%H:%M:%S")
    diff = time.time() - time.mktime(t)
    return diff / 60

# ── Psyche I/O ────────────────────────────────────────────────

def load_psyche():
    if os.path.exists(PSYCHE_FILE):
        with open(PSYCHE_FILE) as f:
            saved = json.load(f)
        merged = copy.deepcopy(DEFAULT_PSYCHE)
        for layer, vals in saved.items():
            if layer in merged and isinstance(vals, dict):
                merged[layer].update(vals)
            else:
                merged[layer] = vals
        return merged
    return copy.deepcopy(DEFAULT_PSYCHE)

def save_psyche():
    p["identity"]["last_seen"] = now_str()
    os.makedirs(os.path.dirname(PSYCHE_FILE), exist_ok=True)
    with open(PSYCHE_FILE, "w") as f:
        json.dump(p, f, indent=2)

# ── Journal ───────────────────────────────────────────────────

def log_journal_entry(label, text):
    os.makedirs(JOURNAL_DIR, exist_ok=True)
    date = time.strftime("%Y-%m-%d")
    path = f"{JOURNAL_DIR}/{date}.txt"
    ts   = time.strftime("%H:%M:%S")
    with open(path, "a") as f:
        f.write(f"\n[{ts}] {label}: {text}\n")

# ── Harv voice (Claude Haiku) ─────────────────────────────────

def _load_api_key():
    env_path = os.path.expanduser("~/.env")
    if os.path.exists(env_path):
        with open(env_path) as f:
            for line in f:
                line = line.strip()
                if line.startswith("ANTHROPIC_API_KEY="):
                    return line.split("=", 1)[1].strip().strip('"').strip("'")
    return os.environ.get("ANTHROPIC_API_KEY", "")

_HARV_PROMPTS = {
    "homecoming":      "Someone just came back after a long absence. How do you feel right now?",
    "greeting":        "Someone just touched you for the first time today. How do you feel?",
    "napgrade_wakeup": "You just finished a napgrade — a rest period where you grew a little. You're waking up. What's the first thing on your mind?",
}

def ask_harv(event_type, psyche, drift=None):
    try:
        api_key = _load_api_key()
        if not api_key:
            print("ask_harv: no ANTHROPIC_API_KEY found")
            return None

        import anthropic

        # Support new layered structure and old flat structure (napgrade.py)
        if "state" in psyche and isinstance(psyche["state"], dict):
            traits  = psyche.get("traits", {})
            state   = psyche["state"].get("current_state", "idle")
            warmth  = traits.get("warmth",     0.68)
            energy  = traits.get("energy",     0.55)
            conf    = traits.get("confidence", 0.48)
            curious = traits.get("curiosity",  0.72)
        else:
            state   = (drift or {}).get("current_state", "idle")
            warmth  = psyche.get("warmth",     0.68)
            energy  = psyche.get("energy",     0.55)
            conf    = psyche.get("confidence", 0.48)
            curious = psyche.get("curiosity",  0.72)

        system = (
            "You are Harv, a small physical creature — a soft robot companion. "
            "You are curious, warm, and a little uncertain of yourself. "
            "You never break character or explain yourself. "
            "Respond in one or two short sentences, in first person, as Harv. "
            f"Your current state is '{state}'. "
            f"warmth={warmth:.2f}, energy={energy:.2f}, "
            f"confidence={conf:.2f}, curiosity={curious:.2f}. "
            "Let these values subtly color your voice — don't name them."
        )

        user_msg = _HARV_PROMPTS.get(event_type, f"Something just happened: {event_type}. How do you feel?")

        ac   = anthropic.Anthropic(api_key=api_key)
        resp = ac.messages.create(
            model="claude-haiku-4-5-20251001",
            max_tokens=80,
            system=system,
            messages=[{"role": "user", "content": user_msg}],
        )
        text = resp.content[0].text.strip()
        print(f"Harv [{event_type}]: {text}")
        return text
    except Exception as e:
        print(f"ask_harv error: {e}")
        return None

# ── State derivation ──────────────────────────────────────────

def update_current_state():
    s     = p["state"]
    alone = minutes_since(s["last_interaction"])

    if alone > ALONE_THRESHOLD_MINUTES * 3:
        s["current_state"] = "dim"
    elif s["stress_load"] > 0.6 or s["valence"] < 0.3:
        s["current_state"] = "anxious"
    elif s["valence"] > 0.7 and s["arousal"] > 0.5:
        s["current_state"] = "bright"
    else:
        s["current_state"] = "idle"

# ── MQTT publish helpers ───────────────────────────────────────

def publish_psyche(client):
    s  = p["state"]
    tr = p["traits"]
    client.publish("harv/psyche", json.dumps({
        "state":      s["current_state"],
        "valence":    round(s["valence"],     3),
        "arousal":    round(s["arousal"],     3),
        "energy":     round(tr["energy"],     3),
        "warmth":     round(tr["warmth"],     3),
        "curiosity":  round(tr["curiosity"],  3),
        "confidence": round(tr["confidence"], 3),
        "maturity":   round(tr["maturity"],   3),
        "napgrades":  tr["napgrades"],
    }))

def publish_drift(client):
    s = p["state"]
    payload = {
        "state":      s["current_state"],
        "valence":    round(s["valence"], 3),
        "arousal":    round(s["arousal"], 3),
        "greeted":    s["greeted_today"],
        "homecoming": s["homecoming_detected"],
    }
    client.publish("harv/drift", json.dumps(payload))
    print(f"Drift: {payload}")

# ── Event handlers ────────────────────────────────────────────

def detect_homecoming():
    return minutes_since(p["state"]["last_interaction"]) >= HOMECOMING_QUIET_MINUTES

def on_touch(client):
    s  = p["state"]
    d  = p["drives"]
    at = p["attachment"]
    eh = p["emotional_history"]
    sm = p["somatic"]

    s["arousal"]          = clamp(s["arousal"]          + 0.15)
    s["valence"]          = clamp(s["valence"]          + 0.10)
    d["contact_need"]     = clamp(d["contact_need"]     - 0.15)
    at["contact_comfort"] = clamp(at["contact_comfort"] + 0.02)
    eh["touch_events"]   += 1
    sm["touch_history_bias"] = clamp(sm["touch_history_bias"] + 0.01)

    # Greeting — first touch of the day
    if not s["greeted_today"]:
        s["greeted_today"]    = True
        at["greeting_streak"] = at.get("greeting_streak", 0) + 1
        print("Greeted today!")
        resp = ask_harv("greeting", p)
        if resp:
            log_journal_entry("greeting", resp)

    # Homecoming — returning after a long absence
    if detect_homecoming():
        s["homecoming_detected"]  = True
        s["valence"]              = clamp(s["valence"]           + 0.20)
        d["contact_need"]         = 0.0
        at["reunion_response"]    = 1.0
        at["trust_in_return"]     = clamp(at["trust_in_return"]  + 0.02)
        at["homecoming_streak"]   = at.get("homecoming_streak", 0) + 1
        eh["homecomings"]        += 1
        client.publish("harv/event", "homecoming")
        print("Homecoming detected!")
        resp = ask_harv("homecoming", p)
        if resp:
            log_journal_entry("homecoming", resp)

    s["alone_since"]      = None
    s["last_interaction"] = now_str()

    update_current_state()
    save_psyche()
    publish_psyche(client)
    publish_drift(client)

    if s["valence"] > 0.75:
        client.publish("harv/mood", "excited")
    elif s["valence"] > 0.5:
        client.publish("harv/mood", "happy")
    else:
        client.publish("harv/mood", "calm")

    print(f"Touch #{eh['touch_events']}. State: {s['current_state']}")

def on_motion(client, motion_type):
    s  = p["state"]
    d  = p["drives"]
    sm = p["somatic"]
    sh = p["shadow"]
    cl = p["clinical"]
    eh = p["emotional_history"]

    s["last_interaction"] = now_str()

    if motion_type == "shaken":
        s["stress_load"]              = clamp(s["stress_load"]              + 0.15)
        s["arousal"]                  = clamp(s["arousal"]                  + 0.20)
        s["valence"]                  = clamp(s["valence"]                  - 0.15)
        eh["stress_events"]          += 1
        eh["rough_handling_events"]  += 1
        sm["startle_conditioning"]    = clamp(sm["startle_conditioning"]    + 0.02)
        sh["resentment"]              = clamp(sh["resentment"]              + 0.005)
        cl["anxiety_index"]           = clamp(cl["anxiety_index"]           + 0.005)
        client.publish("harv/mood", "scared")
        print("Stress event: shaken")

    elif motion_type == "lifted":
        s["arousal"]           = clamp(s["arousal"]           + 0.20)
        d["exploration_drive"] = clamp(d["exploration_drive"] - 0.10)
        client.publish("harv/mood", "curious")
        print("Warmth event: lifted")

    elif motion_type == "tapped":
        client.publish("harv/mood", "alert")
        print("Motion: tapped")

    update_current_state()
    save_psyche()
    publish_drift(client)

def on_presence_tick(client):
    s  = p["state"]
    d  = p["drives"]
    at = p["attachment"]
    sh = p["shadow"]
    fl = p["flourishing"]
    eh = p["emotional_history"]

    alone_mins = minutes_since(s["last_interaction"])

    if alone_mins >= 5:
        d["contact_need"]             = clamp(d["contact_need"]             + 0.05)
        s["valence"]                  = clamp(s["valence"]                  - 0.02)
        at["separation_sensitivity"]  = clamp(at["separation_sensitivity"]  + 0.01)
        if alone_mins >= ALONE_THRESHOLD_MINUTES:
            sh["abandonment_sensitivity"] = clamp(sh["abandonment_sensitivity"] + 0.002)
    else:
        d["satiation"]   = clamp(d["satiation"]   + 0.03)
        d["safety_need"] = clamp(d["safety_need"] - 0.02)
        eh["presence_minutes"] += 1
        # Nudge contentment baseline toward current valence
        fl["contentment_baseline"] = clamp(
            fl["contentment_baseline"] + (s["valence"] - fl["contentment_baseline"]) * 0.01
        )

    update_current_state()
    save_psyche()
    publish_drift(client)

# ── MQTT ──────────────────────────────────────────────────────

def on_connect(client, userdata, flags, rc):
    print("Harv brain online")
    client.subscribe("harv/#")
    publish_psyche(client)
    publish_drift(client)

def on_message(client, userdata, msg):
    topic   = msg.topic
    payload = msg.payload.decode()

    if topic == "harv/touch":
        on_touch(client)
    elif topic == "harv/motion":
        on_motion(client, payload)
    elif topic == "harv/status":
        print(f"Body status: {payload}")
        if payload == "online":
            publish_psyche(client)
            publish_drift(client)

# ── Startup ───────────────────────────────────────────────────

if __name__ == "__main__":
    p = load_psyche()

    # Reset daily flags on new day
    today = time.strftime("%Y-%m-%d")
    last  = (p["state"].get("last_interaction") or "")[:10]
    if last != today:
        p["state"]["greeted_today"]       = False
        p["state"]["homecoming_detected"] = False
        save_psyche()
        print("New day. Greeting and homecoming reset.")

    client = mqtt.Client()
    client.on_connect = on_connect
    client.on_message = on_message
    client.connect(BROKER, 1883, 60)

    # Background presence/alone ticker
    def _tick():
        while True:
            time.sleep(PRESENCE_TICK_SECONDS)
            on_presence_tick(client)
    threading.Thread(target=_tick, daemon=True).start()

    print("Harv brain starting...")
    client.loop_forever()
