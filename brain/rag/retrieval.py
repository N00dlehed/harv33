"""Retrieval pipeline skeleton."""

from __future__ import annotations

from typing import List

from .embed import embed_text
from .index import VectorIndex


def retrieve_relevant(memories: List[str], query: str, top_k: int = 3) -> List[str]:
    """Embed memories and return top matches for query."""
    embeddings = embed_text(memories + [query])
    memory_embeddings = embeddings[:-1]
    query_embedding = embeddings[-1]

    index = VectorIndex()
    for memory, emb in zip(memories, memory_embeddings):
        index.add(emb, memory)

    return index.search(query_embedding, top_k=top_k)
