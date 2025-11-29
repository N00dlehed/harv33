"""Persistent memory store for HARV-33."""

from __future__ import annotations

import json
from collections import deque
from pathlib import Path
from typing import Deque, Dict, List, Optional


class MemoryStore:
    """Simple JSON-backed memory system."""

    def __init__(self, path: Path, max_recent: int = 100) -> None:
        self.path = path
        self.max_recent = max_recent
        self.recent: Deque[Dict[str, object]] = deque(maxlen=max_recent)

    def load(self) -> None:
        """Load memories from disk if present."""
        if self.path.exists():
            with self.path.open("r", encoding="utf-8") as f:
                memories: List[Dict[str, object]] = json.load(f)
                for memory in memories[-self.max_recent :]:
                    self.recent.append(memory)

    def save_memory(self, memory: Dict[str, object]) -> None:
        """Persist a new memory entry and append to recent buffer."""
        self.recent.append(memory)
        all_memories = list(self.recent)
        self.path.parent.mkdir(parents=True, exist_ok=True)
        with self.path.open("w", encoding="utf-8") as f:
            json.dump(all_memories, f, indent=2)

    def recall_recent(self, limit: int = 10) -> List[Dict[str, object]]:
        """Return the most recent memories up to limit."""
        return list(self.recent)[-limit:]

    def recall_tagged(self, tag: str) -> List[Dict[str, object]]:
        """Filter memories containing a given tag key or value."""
        return [m for m in self.recent if tag in m.get("tags", []) or m.get("type") == tag]

    def weighted_retrieval(self, query: str) -> Optional[Dict[str, object]]:
        """Placeholder weighted retrieval using simple keyword matching."""
        for memory in reversed(self.recent):
            if query.lower() in json.dumps(memory).lower():
                return memory
        return None
