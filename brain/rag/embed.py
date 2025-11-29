"""Text embedding utilities stub."""

from __future__ import annotations

from typing import List


def embed_text(chunks: List[str]) -> List[list[float]]:
    """Return dummy embeddings for provided text chunks."""
    return [[len(chunk) % 10 for _ in range(8)] for chunk in chunks]
