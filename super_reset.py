import json
import os

MEMORY_FILE  = "/home/admin/harv_memory.json"
DRIFT_FILE   = "/home/admin/psyche/drift.json"
RAISING_FILE = "/home/admin/psyche/raising_log.json"

BASELINE_PSYCHE = {
    "energy":      0.55,
    "warmth":      0.68,
    "curiosity":   0.72,
    "loyalty":     0.78,
    "confidence":  0.48,
    "maturity":    0.20,
    "napgrades":   0,
    "touch_count": 0,
    "last_seen":   None
}

BASELINE_DRIFT = {
    "current_state":      "idle",
    "baseline_drift":     "warm",
    "warmth_score":       0,
    "stress_score":       0,
    "presence_minutes":   0,
    "last_interaction":   None,
    "alone_since":        None,
    "greeted_today":      False,
    "homecoming_detected": False,
    "age_unit":           0,
    "napgrade_count":     0,
    "drift_history":      []
}

BASELINE_RAISING = {
    "age_unit":              0,
    "warmth_events":         0,
    "stress_events":         0,
    "presence_minutes":      0,
    "greeting_streak":       0,
    "homecoming_streak":     0,
    "rough_handling_events": 0,
    "notes":                 "super reset to baseline"
}

def save(path, data):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w") as f:
        json.dump(data, f, indent=2)
    print(f"Reset: {path}")

print("\n=== SUPER RESET ===\n")
confirm = input("This will wipe all of Harv's memory and growth. Type YES to confirm: ")
if confirm.strip() != "YES":
    print("Aborted.")
else:
    save(MEMORY_FILE,  BASELINE_PSYCHE)
    save(DRIFT_FILE,   BASELINE_DRIFT)
    save(RAISING_FILE, BASELINE_RAISING)
    print("\nHarv has been reset to baseline.")
