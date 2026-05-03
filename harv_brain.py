import paho.mqtt.client as mqtt
import json
import os
import time

BROKER = "localhost"
MEMORY_FILE    = "/home/admin/harv_memory.json"
DRIFT_FILE     = "/home/admin/psyche/drift.json"
RAISING_FILE   = "/home/admin/psyche/raising_log.json"
JOURNAL_DIR    = "/home/admin/psyche/journal"

# Placeholders — calibrate after first week
ALONE_THRESHOLD_MINUTES  = 30   # after this long, Harv notices absence
HOMECOMING_QUIET_MINUTES = 60   # quiet this long before "homecoming" triggers
STRESS_SHAKE_WEIGHT      = 2    # how much a shake hurts
WARMTH_TOUCH_WEIGHT      = 1    # how much a touch helps
PRESENCE_TICK_SECONDS    = 60   # how often to tick presence time

DEFAULT_PSYCHE = {
    "energy": 0.55,
    "warmth": 0.68,
    "curiosity": 0.72,
    "loyalty": 0.78,
    "confidence": 0.48,
    "maturity": 0.20,
    "napgrades": 0,
    "touch_count": 0,
    "last_seen": None
}

DEFAULT_DRIFT = {
    "current_state": "idle",
    "baseline_drift": "warm",
    "warmth_score": 0,
    "stress_score": 0,
    "presence_minutes": 0,
    "last_interaction": None,
    "alone_since": None,
    "greeted_today": False,
    "homecoming_detected": False,
    "age_unit": 0,
    "napgrade_count": 0,
    "drift_history": []
}

DEFAULT_RAISING = {
    "age_unit": 0,
    "warmth_events": 0,
    "stress_events": 0,
    "presence_minutes": 0,
    "greeting_streak": 0,
    "homecoming_streak": 0,
    "rough_handling_events": 0,
    "notes": "placeholders - calibrate after first week"
}

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
    "homecoming":       "Someone just came back after a long absence. How do you feel right now?",
    "greeting":         "Someone just touched you for the first time today. How do you feel?",
    "napgrade_wakeup":  "You just finished a napgrade — a rest period where you grew a little. You're waking up. What's the first thing on your mind?",
}

def ask_harv(event_type, psyche, drift):
    api_key = _load_api_key()
    if not api_key:
        print("ask_harv: no ANTHROPIC_API_KEY found")
        return ""

    import anthropic

    state   = drift.get("current_state", "idle")
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

    try:
        ac = anthropic.Anthropic(api_key=api_key)
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
        return ""

def log_journal_entry(label, text):
    os.makedirs(JOURNAL_DIR, exist_ok=True)
    date = time.strftime("%Y-%m-%d")
    path = f"{JOURNAL_DIR}/{date}.txt"
    ts   = time.strftime("%H:%M:%S")
    with open(path, "a") as f:
        f.write(f"\n[{ts}] {label}: {text}\n")

# ── Load / Save ───────────────────────────────────────────────

def load_json(path, default):
    if os.path.exists(path):
        with open(path, "r") as f:
            return json.load(f)
    return default.copy()

def save_json(path, data):
    with open(path, "w") as f:
        json.dump(data, f, indent=2)

def now_str():
    return time.strftime("%Y-%m-%dT%H:%M:%S")

def minutes_since(timestamp):
    if not timestamp:
        return 999
    t = time.strptime(timestamp, "%Y-%m-%dT%H:%M:%S")
    diff = time.time() - time.mktime(t)
    return diff / 60

# ── Drift logic ───────────────────────────────────────────────

def detect_homecoming():
    mins = minutes_since(drift["last_interaction"])
    return mins >= HOMECOMING_QUIET_MINUTES

def update_drift_state():
    stress  = raising["stress_events"]
    warmth  = raising["warmth_events"]
    alone   = minutes_since(drift["last_interaction"])

    # Simple state rules — placeholder thresholds
    if alone > ALONE_THRESHOLD_MINUTES * 3:
        drift["current_state"] = "dim"
    elif stress > warmth * 2:
        drift["current_state"] = "anxious"
    elif warmth > stress * 2:
        drift["current_state"] = "bright"
    else:
        drift["current_state"] = "idle"

def publish_drift(client):
    payload = {
        "state": drift["current_state"],
        "baseline": drift["baseline_drift"],
        "greeted": drift["greeted_today"],
        "homecoming": drift["homecoming_detected"]
    }
    client.publish("harv/drift", json.dumps(payload))
    print(f"Drift: {payload}")

# ── Event handlers ────────────────────────────────────────────

def on_touch(client):
    psyche["energy"]      = min(1.0, psyche["energy"]   + 0.05)
    psyche["warmth"]      = min(1.0, psyche["warmth"]   + 0.10)
    psyche["touch_count"] = psyche.get("touch_count", 0) + 1

    # First touch of session = greeting
    if not drift["greeted_today"]:
        drift["greeted_today"] = True
        raising["greeting_streak"] = raising.get("greeting_streak", 0) + 1
        print("Greeted today!")
        resp = ask_harv("greeting", psyche, drift)
        if resp:
            log_journal_entry("greeting", resp)

    # Homecoming detection
    if detect_homecoming():
        drift["homecoming_detected"] = True
        raising["homecoming_streak"] = raising.get("homecoming_streak", 0) + 1
        client.publish("harv/event", "homecoming")
        print("Homecoming detected!")
        resp = ask_harv("homecoming", psyche, drift)
        if resp:
            log_journal_entry("homecoming", resp)

    drift["last_interaction"] = now_str()
    raising["warmth_events"]  = raising.get("warmth_events", 0) + WARMTH_TOUCH_WEIGHT
    raising["presence_minutes"] = raising.get("presence_minutes", 0) + 1

    update_drift_state()
    save_json(MEMORY_FILE, psyche)
    save_json(DRIFT_FILE, drift)
    save_json(RAISING_FILE, raising)

    client.publish("harv/psyche", json.dumps(psyche))
    publish_drift(client)

    warmth = psyche["warmth"]
    if warmth > 0.75:
        client.publish("harv/mood", "excited")
    elif warmth > 0.5:
        client.publish("harv/mood", "happy")
    else:
        client.publish("harv/mood", "calm")

    print(f"Touch #{psyche['touch_count']}. State: {drift['current_state']}")

def on_motion(client, motion_type):
    drift["last_interaction"] = now_str()

    if motion_type == "shaken":
        raising["stress_events"] = raising.get("stress_events", 0) + STRESS_SHAKE_WEIGHT
        raising["rough_handling_events"] = raising.get("rough_handling_events", 0) + 1
        client.publish("harv/mood", "scared")
        print("Stress event: shaken")

    elif motion_type == "lifted":
        raising["warmth_events"] = raising.get("warmth_events", 0) + 1
        client.publish("harv/mood", "curious")
        print("Warmth event: lifted")

    elif motion_type == "tapped":
        client.publish("harv/mood", "alert")
        print("Motion: tapped")

    update_drift_state()
    save_json(DRIFT_FILE, drift)
    save_json(RAISING_FILE, raising)
    publish_drift(client)

# ── MQTT ──────────────────────────────────────────────────────

def on_connect(client, userdata, flags, rc):
    print("Harv brain online")
    client.subscribe("harv/#")
    client.publish("harv/psyche", json.dumps(psyche))
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
            client.publish("harv/psyche", json.dumps(psyche))
            publish_drift(client)

# ── Startup ───────────────────────────────────────────────────

if __name__ == "__main__":
    psyche  = load_json(MEMORY_FILE,  DEFAULT_PSYCHE)
    drift   = load_json(DRIFT_FILE,   DEFAULT_DRIFT)
    raising = load_json(RAISING_FILE, DEFAULT_RAISING)

    # Reset daily flags if it's a new day
    today = time.strftime("%Y-%m-%d")
    last  = (drift.get("last_interaction") or "")[:10]
    if last != today:
        drift["greeted_today"]      = False
        drift["homecoming_detected"] = False
        save_json(DRIFT_FILE, drift)
        print("New day. Greeting and homecoming reset.")

    client = mqtt.Client()
    client.on_connect = on_connect
    client.on_message = on_message
    client.connect(BROKER, 1883, 60)

    print("Harv brain starting...")
    client.loop_forever()
