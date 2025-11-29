"""Emotional blending and trigger rules for HARV-33."""

from __future__ import annotations

from dataclasses import dataclass
from typing import Dict

from .state import BrainState


@dataclass
class EmotionWeights:
    """Coefficients used when blending or decaying emotions."""

    blend_factor: float = 0.2
    decay_factor: float = 0.05
    sensor_influence: float = 0.3


class EmotionEngine:
    """Calculates composite emotional responses."""

    def __init__(self, state: BrainState, weights: EmotionWeights | None = None) -> None:
        self.state = state
        self.weights = weights or EmotionWeights()

    def blend(self) -> None:
        """Blend base emotions into mood and engagement."""
        mood_delta = (
            (self.state.trust - self.state.fear) * self.weights.blend_factor
            + self.state.curiosity * 0.1
        )
        engagement_delta = (
            (self.state.curiosity + self.state.trust) * 0.1
            - self.state.fear * 0.05
        )
        self.state.update({"mood": mood_delta, "engagement": engagement_delta})
        self._weighted_decay()

    def process_triggers(self, sensor_data: Dict[str, float]) -> None:
        """Map sensor data to emotional adjustments."""
        deltas: Dict[str, float] = {}
        if sensor_data.get("touch", 0) > 0:
            deltas["trust"] = 0.1 * self.weights.sensor_influence
            deltas["fear"] = -0.05 * self.weights.sensor_influence
        if sensor_data.get("imu_variance", 0) > 0.5:
            deltas["fear"] = deltas.get("fear", 0) + 0.1
        if sensor_data.get("voice_intensity", 0) > 0.7:
            deltas["engagement"] = 0.1
        if sensor_data.get("light_level", 1) < 0.2:
            deltas["curiosity"] = 0.05
        self.state.update(deltas)

    def _weighted_decay(self) -> None:
        """Apply weighted decay that varies per emotional channel."""
        self.state.energy = max(
            0.0, self.state.energy - self.weights.decay_factor * (1 + self.state.engagement)
        )
        for field_name in ["fear", "curiosity", "trust"]:
            value = getattr(self.state, field_name)
            setattr(
                self.state,
                field_name,
                max(0.0, value - self.weights.decay_factor),
            )
