"""Behavior selection engine placeholder."""

from __future__ import annotations

from typing import Dict, List

from .state import BrainState
from .memory import MemoryStore
from mqtt.client import MQTTClient
from mqtt.topics import Topics


class BehaviorEngine:
    """Selects and executes behaviors based on current state and memories."""

    def __init__(self, state: BrainState, memory: MemoryStore) -> None:
        self.state = state
        self.memory = memory
        self.behavior_map: Dict[str, str] = {
            "greet": "body/leds/wave",
            "idle": "body/leds/idle",
            "explore": "body/move/explore",
        }

    def select_behavior(self, perception: List[Dict[str, object]]) -> str:
        """Choose the next behavior using a simple heuristic."""
        if self.state.energy < 0.2:
            return "rest"
        if any(p.get("type") == "sensor" and p.get("data", {}).get("touch") for p in perception):
            return "greet"
        if self.state.curiosity > 0.6:
            return "explore"
        return "idle"

    async def execute(self, behavior: str, mqtt: MQTTClient) -> None:
        """Execute the selected behavior by publishing to MQTT topics."""
        topic = self.behavior_map.get(behavior, Topics.ACTION_IDLE)
        payload = {
            "behavior": behavior,
            "state": self.state.as_dict(),
        }
        await mqtt.publish(topic, payload)
