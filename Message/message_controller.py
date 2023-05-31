#!/usr/bin/env python3

import socket
import configparser
import asyncio
import signal
import sys
import threading
from pprint import pprint

import consul
import hazelcast

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

def get_first_usable_port(st_port):
    test_port = st_port
    while is_port_busy(test_port):
        test_port += 1
    return test_port

def static_vars(**kwargs):
    def decorate(func):
        for k in kwargs:
            setattr(func, k, kwargs[k])
        return func
    return decorate

def sigterm_handler(signum, frame):
    asyncio.get_running_loop().stop()
    message_controller.message_persistence.is_running = False
    client.agent.service.deregister(service_name)
    print("--- Message microservice is off")
    exit(SUCCESS)

class MessageController:
    def __init__(self, mq_name):
        self.hz_client = hazelcast.client.HazelcastClient()
        self.mq = self.hz_client.get_queue(mq_name)
        self.message_persistence = MessagePersistence(self.mq)

    async def handle_connection(self, request):
        SingletonCounter().increment()
        user_messages = self.message_persistence.retrieve()
        print(f"[{SingletonCounter().get_count()}] got GET request -> sending response to the client")
        response_text = "\n".join(user_messages)
        print(response_text)
        return web.Response(text=response_text)

    async def tell_health_status(self, request):
        return web.Response()

    async def accept_connections(self, msg_microservice_port):
        global consumer_thread
        app = web.Application()
        app.router.add_get('/', self.handle_connection)
        app.router.add_get('/health', self.tell_health_status)
        runner = web.AppRunner(app)
        await runner.setup()
        site = web.TCPSite(runner, 'localhost', msg_microservice_port)
        await site.start()
        print(f"--- Message microservice #{msg_microservice_port} is on")
        to_clear_queue = int(sys.argv[1])
        if to_clear_queue:
            self.mq.clear()
        await asyncio.Event().wait()

if __name__ == '__main__':
    for signame in ('SIGINT', 'SIGTERM'):
        signal.signal(getattr(signal, signame), sigterm_handler)

    config = configparser.ConfigParser()
    config.read('../config.cfg')
    start_port = config.getint('DEFAULT', 'start_port')
    mq_name = config.get('MQ_NAME', 'mq_name')

    service_name = 'message_microservice'
    service_port = get_first_usable_port(start_port)
    registration = {
        "name": service_name,
        "port": service_port,
        "check": {
            "http": f"http://localhost:{service_port}/health",
            "interval": "5s"
        }
    }

    client = consul.Consul()

    services = client.agent.services()
    for service_id in services:
        client.agent.service.deregister(service_id)

    # service_id = f"{service_port}"
    # client.agent.service.register(service_id=service_id, **registration)
    client.agent.service.register(**registration)

    registered_services = client.agent.services()

    message_controller = MessageController(mq_name)
    consumer_thread = threading.Thread(target=message_controller.message_persistence.store)
    consumer_thread.start()
    asyncio.run(message_controller.accept_connections(service_port))

