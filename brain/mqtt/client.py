"""Async MQTT client wrapper for HARV-33 brain."""

from __future__ import annotations

import asyncio
import json
from typing import Awaitable, Callable

import asyncio_mqtt


MessageHandler = Callable[[str, bytes], Awaitable[None]]


class MQTTClient:
    """Wraps asyncio-mqtt for publish/subscribe convenience."""

    def __init__(self, broker: str, port: int, client_id: str) -> None:
        self.broker = broker
        self.port = port
        self.client_id = client_id
        self.client = asyncio_mqtt.Client(hostname=broker, port=port, client_id=client_id)
        self.subscriptions: dict[str, MessageHandler] = {}

    async def connect(self) -> None:
        """Connect to the MQTT broker."""
        await self.client.connect()

    async def publish(self, topic: str, payload: dict | str | bytes) -> None:
        """Publish a payload to a topic."""
        if isinstance(payload, dict):
            data = json.dumps(payload).encode()
        elif isinstance(payload, str):
            data = payload.encode()
        else:
            data = payload
        await self.client.publish(topic, data)

    async def subscribe(self, topic: str, handler: MessageHandler) -> None:
        """Subscribe to a topic and register a handler."""
        self.subscriptions[topic] = handler
        await self.client.subscribe(topic)
        asyncio.create_task(self._listen(topic, handler))

    async def _listen(self, topic: str, handler: MessageHandler) -> None:
        """Internal listener loop forwarding messages to handler."""
        async with self.client.unfiltered_messages() as messages:
            async for message in messages:
                if message.topic == topic:
                    await handler(message.topic, message.payload)
