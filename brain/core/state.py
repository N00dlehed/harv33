"""State management for HARV-33."""

from __future__ import annotations

from dataclasses import dataclass, field
from typing import Dict


@dataclass
class BrainState:
    """Represents HARV's dynamic internal variables."""

    energy: float = 1.0
    mood: float = 0.5
    curiosity: float = 0.5
    trust: float = 0.5
    fear: float = 0.1
    engagement: float = 0.5
    decay_rate: float = field(default=0.01, repr=False)

    @classmethod
    def from_config(cls, config: Dict[str, float]) -> "BrainState":
        """Create a BrainState using values from configuration."""
        return cls(
            energy=config.get("energy", 1.0),
            mood=config.get("mood", 0.5),
            curiosity=config.get("curiosity", 0.5),
            trust=config.get("trust", 0.5),
            fear=config.get("fear", 0.1),
            engagement=config.get("engagement", 0.5),
        )

    def update_from_sensors(self, sensor_data: Dict[str, float]) -> None:
        """Update state values based on incoming sensor data."""
        self.energy = max(0.0, min(1.0, self.energy - sensor_data.get("load", 0.0)))
        self.engagement = max(0.0, min(1.0, self.engagement + sensor_data.get("attention", 0.0)))
        self.fear = max(0.0, min(1.0, self.fear + sensor_data.get("threat", 0.0)))

    def decay(self) -> None:
        """Apply passive decay to dynamic variables."""
        for field_name in ["energy", "mood", "curiosity", "trust", "fear", "engagement"]:
            value = getattr(self, field_name)
            setattr(self, field_name, max(0.0, min(1.0, value - self.decay_rate)))

    def update(self, deltas: Dict[str, float]) -> None:
        """Apply direct deltas to state variables."""
        for key, delta in deltas.items():
            if hasattr(self, key):
                new_value = getattr(self, key) + delta
                setattr(self, key, max(0.0, min(1.0, new_value)))

    def as_dict(self) -> Dict[str, float]:
        """Return a dictionary representation of the state."""
        return {
            "energy": self.energy,
            "mood": self.mood,
            "curiosity": self.curiosity,
            "trust": self.trust,
            "fear": self.fear,
            "engagement": self.engagement,
        }
