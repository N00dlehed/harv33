"""
Main application entry for HARV-33 brain.
Coordinates configuration loading, MQTT setup, state initialization,
and the cognition loop that runs HARV's perception-behavior cycle.
"""

from __future__ import annotations

import asyncio
import json
from pathlib import Path
from typing import Any, Dict

import yaml

from core.behavior import BehaviorEngine
from core.emotions import EmotionEngine
from core.memory import MemoryStore
from core.state import BrainState
from mqtt.client import MQTTClient
from mqtt.topics import Topics
from napgrade.sleep_cycle import NapCycle


CONFIG_PATH = Path(__file__).parent / "config.yaml"
MEMORY_PATH = Path(__file__).parent.parent / "data" / "memories.json"


class HARVBrain:
    """HARV-33 brain orchestrating state, memory, and behavior."""

    def __init__(self, config: Dict[str, Any]) -> None:
        self.config = config
        self.state = BrainState.from_config(config.get("state", {}))
        self.memory = MemoryStore(MEMORY_PATH)
        self.emotions = EmotionEngine(self.state)
        self.behavior = BehaviorEngine(self.state, self.memory)
        self.mqtt = MQTTClient(
            broker=config["mqtt"].get("host", "localhost"),
            port=config["mqtt"].get("port", 1883),
            client_id=config["mqtt"].get("client_id", "harv-brain"),
        )
        self.nap_cycle = NapCycle(self.state, self.memory)

    async def setup(self) -> None:
        """Initialize MQTT subscriptions and ensure memory is loaded."""
        await self.mqtt.connect()
        await self.mqtt.subscribe(Topics.SENSORS_ALL, self.handle_sensor_payload)
        await self.mqtt.subscribe(Topics.BRIDGE_UART_IN, self.handle_bridge_payload)
        self.memory.load()

    async def handle_sensor_payload(self, topic: str, payload: bytes) -> None:
        """Process raw sensor input from body modules."""
        data = json.loads(payload.decode())
        self.emotions.process_triggers(data)
        self.state.update_from_sensors(data)
        self.memory.save_memory({"type": "sensor", "data": data})

    async def handle_bridge_payload(self, topic: str, payload: bytes) -> None:
        """Process UART bridge messages forwarded via MQTT."""
        data = payload.decode()
        self.memory.save_memory({"type": "bridge", "data": data})

    async def cognition_loop(self) -> None:
        """Run perception → state update → behavior selection → action → logging."""
        interval = self.config.get("loop_interval_ms", 200) / 1000
        while True:
            perception = self.memory.recall_recent(limit=5)
            self.state.decay()
            self.emotions.blend()
            action = self.behavior.select_behavior(perception)
            await self.behavior.execute(action, self.mqtt)
            self.memory.save_memory({
                "type": "action",
                "name": action,
                "state": self.state.as_dict(),
            })
            await asyncio.sleep(interval)

    async def nightly_napgrade(self) -> None:
        """Trigger nightly consolidation based on configured hour."""
        await self.nap_cycle.run_cycle()


def load_config() -> Dict[str, Any]:
    """Load configuration from YAML file."""
    with CONFIG_PATH.open("r", encoding="utf-8") as f:
        return yaml.safe_load(f)


async def main() -> None:
    config = load_config()
    brain = HARVBrain(config)
    await brain.setup()

    cognition = asyncio.create_task(brain.cognition_loop())
    napgrade = asyncio.create_task(brain.nightly_napgrade())

    await asyncio.gather(cognition, napgrade)


if __name__ == "__main__":
    asyncio.run(main())
