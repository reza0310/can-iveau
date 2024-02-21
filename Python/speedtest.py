import time
from caniveau import Caniveau, hex


TIME = 100
caniveau = Caniveau("can-iveau", 1, 1, 1000)
start = time.time()
counter = 0
caniveau.send_parsed_checked(0, 1, 42, 3, 0, 42, "DEADBEEFDEADBEEF")
while time.time()-start < TIME:
    i = time.time()
    x = caniveau.receive_parsed()
    while not x[7]:
        x = caniveau.receive_parsed()
        if time.time()-i > 1:
            caniveau.stop()
            raise Exception("TIMEOUT")
    if len(caniveau.queue) > 0:
        raise Exception("Received twice the same data")
    caniveau.send_parsed_checked(0, 1, 42, 3, 0, 42, hex(int(x[6], 16)+3))
    counter += 1
print("Taille de file (devrait être 0):", len(caniveau.queue))
caniveau.stop()
print("Process terminé correctement")
print("Obtention de", counter, "échanges de 8 bytes allé-retour en "+str(TIME)+" secondes soit une moyenne de", counter/TIME, "échanges ou", (counter/TIME)*8*8, "bits/seconde dans les deux sens.")

