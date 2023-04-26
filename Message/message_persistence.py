#!/usr/bin/env python3
import typing
from hazelcast.proxy import Queue
from hazelcast.proxy.base import ItemEvent
from message_model import SingletonCounter

class MessagePersistence:
    def __init__(self, message_queue):
        self.messages = []
        self.message_queue: Queue = message_queue
        self.is_running = True

    def store(self):
        while self.is_running:
            message = str(self.message_queue.take().result())
            SingletonCounter().increment()
            print(f"[{SingletonCounter().get_count()}] Retrieved user data from message queue ->"
                  f" saving it to memory")
            print(message)
            self.messages.append(message)

    def retrieve(self) -> typing.List[str]:
        return self.messages
