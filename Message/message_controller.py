#!/usr/bin/env python3

import asyncio

import signal
from aiohttp import web
ONE_KB = 1024

def static_vars(**kwargs):
    def decorate(func):
        for k in kwargs:
            setattr(func, k, kwargs[k])
        return func
    return decorate

def sigterm_handler(signum, frame):
    loop = asyncio.get_running_loop()
    loop.stop()
    print("--- Message microservice is off")
    exit(0)

class MessageController:
    @staticmethod
    @static_vars(counter=0)
    async def handle_connection(request):
        MessageController.handle_connection.counter += 1
        print(f"[{MessageController.handle_connection.counter}] got GET request -> sending response to the client")
        response_text = "not implemented yet"
        return web.Response(text=response_text)

    @staticmethod
    async def accept_connections():
        app = web.Application()
        app.router.add_get('/', MessageController.handle_connection)
        runner = web.AppRunner(app)
        await runner.setup()
        site = web.TCPSite(runner, 'localhost', 2001)
        await site.start()
        print("--- Message microservice is on")
        await asyncio.Event().wait()

if __name__ == '__main__':
    for signame in ('SIGINT', 'SIGTERM'):
        signal.signal(getattr(signal, signame), sigterm_handler)
    message_controller = MessageController()
    asyncio.run(message_controller.accept_connections())
