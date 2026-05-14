"""
harv_monitor.py — real-time terminal dashboard for Harv's inner state.
Subscribes to all harv/* MQTT topics and prints a live feed.
"""

import json
import time
import paho.mqtt.client as mqtt

BROKER = "localhost"

# ANSI colours
R  = "\033[0;31m"
G  = "\033[0;32m"
Y  = "\033[0;33m"
B  = "\033[0;34m"
M  = "\033[0;35m"
C  = "\033[0;36m"
W  = "\033[0;37m"
DIM= "\033[2m"
RST= "\033[0m"
BOLD="\033[1m"

STATE_COLOURS = {
    "bright":  Y,
    "idle":    B,
    "anxious": R,
    "dim":     DIM,
}

def ts():
    return time.strftime("%H:%M:%S")

def bar(val, width=20, colour=G):
    filled = int(val * width)
    return colour + "█" * filled + DIM + "░" * (width - filled) + RST

def fmt_val(v):
    try:
        f = float(v)
        colour = G if f >= 0.6 else Y if f >= 0.35 else R
        return f"{colour}{f:.2f}{RST}"
    except (TypeError, ValueError):
        return str(v)

def on_connect(client, userdata, flags, rc):
    print(f"{G}[{ts()}] Connected to MQTT broker{RST}")
    print(f"{DIM}Listening on harv/#  —  Ctrl-C to exit{RST}\n")
    client.subscribe("harv/#")

def on_message(client, userdata, msg):
    topic   = msg.topic
    payload = msg.payload.decode()
    t       = ts()

    # ── harv/psyche ───────────────────────────────────────────
    if topic == "harv/psyche":
        try:
            d = json.loads(payload)
        except json.JSONDecodeError:
            print(f"{DIM}[{t}] harv/psyche  (bad JSON){RST}")
            return

        state  = d.get("state", "?")
        sc     = STATE_COLOURS.get(state, W)

        print(f"\n{BOLD}{'─'*55}{RST}")
        print(f"  {BOLD}PSYCHE{RST}  [{t}]   state: {sc}{BOLD}{state}{RST}")
        print(f"{'─'*55}")

        fields = ["energy", "warmth", "curiosity", "confidence", "maturity"]
        for k in fields:
            v = d.get(k)
            if v is None:
                continue
            try:
                f = float(v)
                colour = G if f >= 0.6 else Y if f >= 0.35 else R
                print(f"  {k:<12} {bar(f, 18, colour)}  {colour}{f:.3f}{RST}")
            except (TypeError, ValueError):
                pass

        extras = {k: v for k, v in d.items()
                  if k not in ("state", *fields)}
        if extras:
            parts = "  ".join(f"{k}={fmt_val(v)}" for k, v in extras.items())
            print(f"  {DIM}{parts}{RST}")
        print()

    # ── harv/drift ────────────────────────────────────────────
    elif topic == "harv/drift":
        try:
            d = json.loads(payload)
        except json.JSONDecodeError:
            print(f"{DIM}[{t}] harv/drift  (bad JSON){RST}")
            return

        state = d.get("state", "?")
        sc    = STATE_COLOURS.get(state, W)
        flags = []
        if d.get("greeted"):
            flags.append(f"{G}greeted{RST}")
        if d.get("homecoming"):
            flags.append(f"{M}homecoming{RST}")
        flag_str = "  " + "  ".join(flags) if flags else ""

        valence = d.get("valence", "")
        arousal = d.get("arousal", "")
        vals = ""
        if valence != "" and arousal != "":
            vals = f"  valence={fmt_val(valence)}  arousal={fmt_val(arousal)}"

        print(f"  {C}DRIFT{RST}  [{t}]  {sc}{state}{RST}{vals}{flag_str}")

    # ── harv/mood ─────────────────────────────────────────────
    elif topic == "harv/mood":
        mood_colours = {
            "excited": Y, "thrilled": Y, "giddy": Y, "playful": Y,
            "happy": G,   "content": G, "pleased": G, "warm": G,
            "calm": B,    "curious": C, "alert": C,
            "scared": R,  "anxious": R, "panicked": R, "distressed": R,
            "sad": DIM,   "dim": DIM,   "slow": DIM,
        }
        c = mood_colours.get(payload, W)
        print(f"  {BOLD}MOOD{RST}    [{t}]  {c}{BOLD}{payload}{RST}")

    # ── harv/idle ─────────────────────────────────────────────
    elif topic == "harv/idle":
        print(f"  {DIM}IDLE    [{t}]  {payload}{RST}")

    # ── harv/event ────────────────────────────────────────────
    elif topic == "harv/event":
        print(f"  {M}{BOLD}EVENT{RST}   [{t}]  {M}{payload}{RST}")

    # ── harv/touch / harv/motion ──────────────────────────────
    elif topic == "harv/touch":
        print(f"  {G}TOUCH{RST}   [{t}]")
    elif topic == "harv/motion":
        c = R if payload == "shaken" else C if payload == "lifted" else W
        print(f"  {c}MOTION{RST}  [{t}]  {c}{payload}{RST}")

    # ── harv/status ───────────────────────────────────────────
    elif topic == "harv/status":
        print(f"  {G}STATUS{RST}  [{t}]  {payload}")

    # ── harv/sim/* ────────────────────────────────────────────
    elif topic.startswith("harv/sim/"):
        print(f"  {DIM}SIM     [{t}]  {topic}  {payload}{RST}")


client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message

print(f"{BOLD}Harv Monitor{RST}")
client.connect(BROKER, 1883, 60)
try:
    client.loop_forever()
except KeyboardInterrupt:
    print(f"\n{DIM}Monitor stopped.{RST}")
