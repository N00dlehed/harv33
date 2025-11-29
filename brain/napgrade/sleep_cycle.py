"""Napgrade scheduling stub."""

from __future__ import annotations

import asyncio
from datetime import datetime, time

from .consolidate import consolidate_memories
from ..core.state import BrainState
from ..core.memory import MemoryStore


class NapCycle:
    """Runs nightly napgrade routines."""

    def __init__(self, state: BrainState, memory: MemoryStore, target_hour: int = 3) -> None:
        self.state = state
        self.memory = memory
        self.target_hour = target_hour

    async def run_cycle(self) -> None:
        """Wait until target hour each day and run consolidation."""
        while True:
            await self._sleep_until_target()
            consolidate_memories(self.state, self.memory)

    async def _sleep_until_target(self) -> None:
        """Sleep until the configured target hour."""
        now = datetime.now()
        target = datetime.combine(now.date(), time(self.target_hour, 0))
        if now >= target:
            target = datetime.combine(now.date(), time(self.target_hour, 0))
            target = target.replace(day=target.day + 1)
        await asyncio.sleep((target - now).total_seconds())
