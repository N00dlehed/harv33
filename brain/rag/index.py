"""Vector index placeholder."""

from __future__ import annotations

from typing import List


class VectorIndex:
    """In-memory vector index stub."""

    def __init__(self) -> None:
        self.vectors: List[list[float]] = []
        self.documents: List[str] = []

    def add(self, embedding: list[float], document: str) -> None:
        """Add embedding and associated document."""
        self.vectors.append(embedding)
        self.documents.append(document)

    def search(self, query_vector: list[float], top_k: int = 3) -> List[str]:
        """Return top_k documents with naive scoring."""
        scores = [sum(v) for v in self.vectors]
        sorted_docs = [doc for _, doc in sorted(zip(scores, self.documents), reverse=True)]
        return sorted_docs[:top_k]
