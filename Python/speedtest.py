import time
from caniveau import Caniveau


TIME = 100
caniveau = Caniveau("can-iveau", 1, 1, 50)
start = time.time()
counter = 0
caniveau.send_parsed_checked(0, 1, 42, 3, 0, 42, [0xDE, 0xAD, 0xBE, 0xEF, 0xDE, 0xAD, 0xBE, 0xEF])
while time.time()-start < TIME:
    x = caniveau.receive_parsed()
    while not x[7]:
        x = caniveau.receive_parsed()
    ans = [num+3 for num in x[6]]
    caniveau.send_parsed_checked(0, 1, 42, 3, 0, 42, ans)
    counter += 1
print("Taille de file (devrait être 0):", len(caniveau.queue))
caniveau.stop()
print("Process terminé correctement")
print("Obtention de", counter, "échanges de 8 bytes allé-retour en "+str(TIME)+" secondes soit une moyenne de", counter/TIME, "échanges ou", (counter/TIME)*8*8, "bits/seconde dans les deux sens.")

