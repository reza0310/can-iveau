import subprocess
import threading
import time
import os
from can_queue import can_queue, can_data


if "not found" in subprocess.run(["candump", "-h"], capture_output=True, text=True).stdout:
    raise Exception("can-utils isn't installed !")
if "not found" in subprocess.run(["ifconfig", "-h"], capture_output=True, text=True).stdout:
    raise Exception("net-tools isn't installed !")
if "not found" in subprocess.run(["ifconfig", "can-iveau"], capture_output=True, text=True).stdout:
    raise Exception("can-iveau isn't set up properly !")
print("Configuration vérifiée.")


finished = threading.Event()
QUEUE = can_queue(50, True)
HEADER = "1FFE30AA"
TIME = 100


def gather():
    dump = subprocess.Popen(["candump", "-t", "A", "can-iveau"], stdout=subprocess.PIPE, text=True)
    os.set_blocking(dump.stdout.fileno(), False)
    while not finished.is_set():
        msg = dump.stdout.readline()  # Bloquant si on retire le set_blocking au dessus
        if msg and HEADER not in msg:
            x = msg.split("  ")
            QUEUE.add(can_data(x[1], x[3]))
    dump.kill()


gatherer = threading.Thread(target=gather)
gatherer.start()
start = time.time()
counter = 0
while time.time()-start < TIME:
    subprocess.run(["cansend", "can-iveau", HEADER+"#1122334455667788"])
    i = time.time()
    while len(QUEUE) < 1:
        if time.time()-i > 1:
            finished.set()
            gatherer.join()
            raise Exception("TIMEOUT")
    QUEUE.pop()
    counter += 1
print("Process terminé correctement")
finished.set()
time.sleep(0.5)  # On sleep au lieu de join pcq c'est probablement bloqué...
print("Taille de file (devrait être 0):", len(QUEUE))
print("Obtention de", counter, "échanges de 8 bytes allé-retour en "+str(TIME)+" secondes soit une moyenne de", counter/TIME, "échanges ou", (counter/TIME)*8*8, "bits/seconde dans les deux sens.")

