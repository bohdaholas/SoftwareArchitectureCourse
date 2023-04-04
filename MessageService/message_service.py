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

@static_vars(counter=0)
async def handle(request):
    handle.counter += 1
    print(f"[{handle.counter}] got GET request -> sending response to the client")
    response_text = "not implemented yet"
    return web.Response(text=response_text)

async def main():
    app = web.Application()
    app.router.add_get('/', handle)
    runner = web.AppRunner(app)
    await runner.setup()
    site = web.TCPSite(runner, 'localhost', 2002)
    await site.start()
    print("--- MessageService is on")
    await asyncio.Event().wait()

def sigterm_handler(signum, frame):
    loop = asyncio.get_running_loop()
    loop.stop()
    print("--- MessageService is off")
    exit(0)

if __name__ == '__main__':
    for signame in ('SIGINT', 'SIGTERM'):
        signal.signal(getattr(signal, signame), sigterm_handler)
    asyncio.run(main())
