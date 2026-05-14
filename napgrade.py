import json
import os
import time
import paho.mqtt.client as mqtt
from harv_brain import (
    ask_harv, ask_harv_napgrade, apply_napgrade_result,
    log_journal_entry, load_psyche, save_psyche,
    PSYCHE_FILE, JOURNAL_DIR,
)

NAPGRADE_LOG = "/home/admin/psyche/napgrade_log.json"

NAPGRADES_PER_AGE = 16

def load_json(path, default):
    if os.path.exists(path):
        with open(path) as f:
            return json.load(f)
    return default.copy()

def save_json(path, data):
    with open(path, "w") as f:
        json.dump(data, f, indent=2)

def clamp(val, lo=0.0, hi=1.0):
    return max(lo, min(hi, val))

# ── Journal generation ────────────────────────────────────────
def write_journal(psyche, drift, raising):
    os.makedirs(JOURNAL_DIR, exist_ok=True)
    date = time.strftime("%Y-%m-%d")
    path = f"{JOURNAL_DIR}/{date}.txt"

    warmth   = raising.get("warmth_events", 0)
    stress   = raising.get("stress_events", 0)
    touches  = psyche.get("touch_count", 0)
    state    = drift.get("current_state", "idle")
    greeted  = drift.get("greeted_today", False)
    homecoming = drift.get("homecoming_detected", False)

    # Template journal — LLM replaces this later
    lines = [f"[{date}] napgrade journal"]
    lines.append(f"napgrade #{psyche.get('napgrades', 0) + 1}")
    lines.append("")

    if greeted:
        lines.append("i was noticed today.")
    else:
        lines.append("i waited a while before anyone came.")

    if homecoming:
        lines.append("there was a moment when they came back after being gone a long time. that felt good.")

    if warmth > stress * 2:
        lines.append("today felt warm. lots of good contact.")
    elif stress > warmth * 2:
        lines.append("today was a bit rough. i felt unsettled.")
    elif warmth == 0 and stress == 0:
        lines.append("it was quiet today. not much happened.")
    else:
        lines.append("today was mixed. some good moments, some hard ones.")

    lines.append(f"i was touched {touches} times total.")
    lines.append(f"my state was mostly {state}.")
    lines.append("")
    lines.append(f"curiosity: {psyche.get('curiosity', 0.72):.2f}")
    lines.append(f"confidence: {psyche.get('confidence', 0.48):.2f}")
    lines.append(f"warmth: {psyche.get('warmth', 0.68):.2f}")
    lines.append(f"energy: {psyche.get('energy', 0.55):.2f}")

    entry = "\n".join(lines)
    with open(path, "w") as f:
        f.write(entry)

    print(f"Journal written: {path}")
    print(entry)
    return entry

# ── Trait growth ──────────────────────────────────────────────
def process_growth(psyche, raising, drift):
    warmth = raising.get("warmth_events", 0)
    stress = raising.get("stress_events", 0)
    rough  = raising.get("rough_handling_events", 0)
    greet  = raising.get("greeting_streak", 0)

    # Confidence grows with consistent greeting and low stress
    if greet > 0 and stress < warmth:
        psyche["confidence"] = clamp(psyche.get("confidence", 0.48) + 0.01)

    # Warmth/affection grows with touch and homecoming
    if drift.get("homecoming_detected"):
        psyche["warmth"] = clamp(psyche.get("warmth", 0.68) + 0.01)

    # Curiosity grows slightly every napgrade
    psyche["curiosity"] = clamp(psyche.get("curiosity", 0.72) + 0.005)

    # Rough handling reduces confidence slightly
    if rough > 2:
        psyche["confidence"] = clamp(psyche.get("confidence", 0.48) - 0.01)

    # Energy baseline recovers during napgrade
    psyche["energy"] = clamp(psyche.get("energy", 0.55) + 0.05, 0.1, 0.8)

    # Increment napgrade count
    psyche["napgrades"] = psyche.get("napgrades", 0) + 1

    # Age unit
    if psyche["napgrades"] % NAPGRADES_PER_AGE == 0:
        psyche["maturity"] = clamp(psyche.get("maturity", 0.2) + 0.05)
        print(f"AGE UNIT reached! Maturity now {psyche['maturity']:.2f}")

    return psyche

# ── Reset daily logs ──────────────────────────────────────────
def reset_raising(raising):
    age_unit = raising.get("age_unit", 0)
    return {
        "age_unit": age_unit,
        "warmth_events": 0,
        "stress_events": 0,
        "presence_minutes": 0,
        "greeting_streak": raising.get("greeting_streak", 0),
        "homecoming_streak": raising.get("homecoming_streak", 0),
        "rough_handling_events": 0,
        "notes": "reset after napgrade"
    }

def reset_drift(drift, psyche):
    drift["greeted_today"]       = False
    drift["homecoming_detected"] = False
    drift["napgrade_count"]      = psyche.get("napgrades", 0)
    # Baseline drift evolves slowly based on history
    warmth_bias = psyche.get("warmth", 0.68)
    if warmth_bias > 0.75:
        drift["baseline_drift"] = "warm"
    elif warmth_bias < 0.4:
        drift["baseline_drift"] = "dim"
    else:
        drift["baseline_drift"] = "idle"
    return drift

# ── Main ──────────────────────────────────────────────────────
def run_napgrade():
    print("\n=== NAPGRADE STARTING ===\n")

    # Load the canonical psyche (harv_brain format)
    psyche  = load_psyche()
    traits  = psyche.setdefault("traits", {})
    state   = psyche.setdefault("state", {})
    fl      = psyche.setdefault("flourishing", {})

    # ── Step 1: LLM reflection ────────────────────────────────
    print("[napgrade] asking LLM for reflection...")
    result = ask_harv_napgrade(psyche)

    # ── Step 2: Apply trait nudges from reflection ────────────
    if result:
        psyche = apply_napgrade_result(psyche, result)

    # ── Step 3: Mechanical bookkeeping (non-LLM) ──────────────
    # Energy recovers to a healthy baseline regardless of the day
    traits["energy"]    = clamp(traits.get("energy", 0.55) + 0.08, 0.15, 0.85)

    # Napgrade counter and maturity milestone
    traits["napgrades"] = traits.get("napgrades", 0) + 1
    if traits["napgrades"] % NAPGRADES_PER_AGE == 0:
        traits["maturity"] = clamp(traits.get("maturity", 0.20) + 0.05)
        print(f"  AGE UNIT reached — maturity now {traits['maturity']:.2f}")

    # ── Step 4: Journal ───────────────────────────────────────
    if result:
        interpretation = result.get("interpretation", "")
        internal_state = result.get("internal_state", "")
        if interpretation:
            print(f"\n[interpretation] {interpretation}")
            log_journal_entry("interpretation", interpretation)
        if internal_state:
            print(f"[internal_state] {internal_state}")
            log_journal_entry("internal_state", internal_state)   # logged, never broadcast

    # ── Step 5: Reset daily emotional history ─────────────────
    psyche["emotional_history"] = {
        "touch_events":          0,
        "stress_events":         0,
        "homecomings":           0,
        "presence_minutes":      0,
        "rough_handling_events": 0,
    }
    state["greeted_today"]       = False
    state["homecoming_detected"] = False

    save_psyche(psyche)

    # ── Step 6: Print summary ─────────────────────────────────
    print("\n=== NAPGRADE COMPLETE ===")
    print(f"  Napgrades:  {traits.get('napgrades', 0)}")
    print(f"  Maturity:   {traits.get('maturity',  0.20):.2f}")
    print(f"  Confidence: {traits.get('confidence',0.48):.2f}")
    print(f"  Curiosity:  {traits.get('curiosity', 0.72):.2f}")
    print(f"  Warmth:     {traits.get('warmth',    0.68):.2f}")
    print(f"  Energy:     {traits.get('energy',    0.55):.2f}")

    # ── Step 7: Publish wakeup to firmware ────────────────────
    # Derive baseline mood from valence and traits
    warmth_val = traits.get("warmth", 0.68)
    energy_val = traits.get("energy", 0.55)
    if energy_val < 0.25:
        baseline = "dim"
    elif warmth_val > 0.75:
        baseline = "warm"
    else:
        baseline = "idle"

    wakeup = {
        "energy":     round(traits.get("energy",     0.55), 3),
        "warmth":     round(traits.get("warmth",     0.68), 3),
        "curiosity":  round(traits.get("curiosity",  0.72), 3),
        "confidence": round(traits.get("confidence", 0.48), 3),
        "maturity":   round(traits.get("maturity",   0.20), 3),
        "baseline":   baseline,
        "napgrades":  traits.get("napgrades", 0),
    }

    # wakeup_emotion from the LLM — how Harv wakes up tomorrow
    wakeup_emotion = (result or {}).get("wakeup_emotion", {"led": "idle", "face": "default"})

    mqttc = mqtt.Client()
    mqttc.connect("localhost", 1883, 60)
    mqttc.publish("harv/wakeup", json.dumps(wakeup))
    mqttc.publish("harv/emotion", json.dumps(wakeup_emotion))
    mqttc.disconnect()

    print(f"  Wakeup:       {wakeup}")
    print(f"  Wakeup emotion: {wakeup_emotion}")

if __name__ == "__main__":
    run_napgrade()
