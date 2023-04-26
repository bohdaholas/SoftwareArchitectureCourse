#!/usr/bin/env python3

import socket
import configparser
import asyncio
import signal
import sys
import threading

from hazelcast import client
from aiohttp import web
from message_persistence import MessagePersistence
from message_model import SingletonCounter

ONE_KB = 1024
SUCCESS = 0
FAILURE = 1

consumer_thread = None

def is_port_busy(port):
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        try:
            s.bind(('localhost', port))
            return False
        except socket.error:
            return True

def static_vars(**kwargs):
    def decorate(func):
        for k in kwargs:
            setattr(func, k, kwargs[k])
        return func
    return decorate

def sigterm_handler(signum, frame):
    asyncio.get_running_loop().stop()
    message_controller.message_persistence.is_running = False
    print("--- Message microservice is off")
    exit(SUCCESS)

class MessageController:
    def __init__(self, mq_name):
        self.hz_client = client.HazelcastClient()
        self.mq = self.hz_client.get_queue(mq_name)
        self.message_persistence = MessagePersistence(self.mq)

    async def handle_connection(self, request):
        SingletonCounter().increment()
        user_messages = self.message_persistence.retrieve()
        print(f"[{SingletonCounter().get_count()}] got GET request -> sending response to the client")
        response_text = "\n".join(user_messages)
        print(response_text)
        return web.Response(text=response_text)

    async def accept_connections(self, msg_microservice_port, msg_microservices_count):
        global consumer_thread
        app = web.Application()
        app.router.add_get('/', self.handle_connection)
        runner = web.AppRunner(app)
        await runner.setup()
        site = None
        port = 0
        for test_port in range(msg_microservice_port, msg_microservice_port + msg_microservices_count):
            if not is_port_busy(test_port):
                site = web.TCPSite(runner, 'localhost', test_port)
                port = test_port
                break
        if site is None:
            print("[!] Failed to launch message microservice")
            exit(FAILURE)
        await site.start()
        print(f"--- Message microservice #{port} is on")
        to_clear_queue = int(sys.argv[1])
        if to_clear_queue:
            self.mq.clear()
        await asyncio.Event().wait()

if __name__ == '__main__':
    for signame in ('SIGINT', 'SIGTERM'):
        signal.signal(getattr(signal, signame), sigterm_handler)

    config = configparser.ConfigParser()
    config.read('../config.cfg')
    message_microservice_port = config.getint('DEFAULT', 'message_microservice_port')
    message_microservices_count = config.getint('DEFAULT', 'message_microservices_count')
    mq_name = config.get('MQ_NAME', 'mq_name')

    message_controller = MessageController(mq_name)
    consumer_thread = threading.Thread(target=message_controller.message_persistence.store)
    consumer_thread.start()
    asyncio.run(message_controller.accept_connections(message_microservice_port,
                                                      message_microservices_count))

