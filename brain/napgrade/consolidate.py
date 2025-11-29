"""Napgrade consolidation logic placeholder."""

from __future__ import annotations

from typing import List

from ..core.state import BrainState
from ..core.memory import MemoryStore
from ..rag.retrieval import retrieve_relevant


def consolidate_memories(state: BrainState, memory: MemoryStore) -> List[str]:
    """Perform memory consolidation and emotional reset."""
    memories = [str(m) for m in memory.recall_recent(limit=50)]
    summaries = retrieve_relevant(memories, query="daily reflection")
    state.energy = min(1.0, state.energy + 0.2)
    state.fear = max(0.0, state.fear - 0.1)
    return summaries
