import copy
import json
import os

from harv_brain import DEFAULT_PSYCHE, PSYCHE_FILE

print("\n=== SUPER RESET ===\n")
confirm = input("This will wipe all of Harv's memory and growth. Type YES to confirm: ")
if confirm.strip() != "YES":
    print("Aborted.")
else:
    baseline = copy.deepcopy(DEFAULT_PSYCHE)
    os.makedirs(os.path.dirname(PSYCHE_FILE), exist_ok=True)
    with open(PSYCHE_FILE, "w") as f:
        json.dump(baseline, f, indent=2)
    print(f"Reset: {PSYCHE_FILE}")
    print("\nHarv has been reset to baseline.")
