import os
from threading import Thread

import swapit_module_server


# generic callback using kwargs
def callback1(**kwargs) -> bool:
    print(kwargs)
    return True


# typed callback
def callback2(order: str, speed: float, plot: bool) -> bool:
    print(f"order: {order}, speed: {speed}, plot: {plot}")
    return True


working_dir = os.path.dirname(__file__)
config1 = os.path.join(working_dir, "config1.json")
config2 = os.path.join(working_dir, "config2.json")

t1 = Thread(target=swapit_module_server.run, args=(config1, callback1, True))
t2 = Thread(target=swapit_module_server.run, args=(config2, callback2, True))
t1.start()
t2.start()
t1.join()
t2.join()
