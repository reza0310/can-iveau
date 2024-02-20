from queue import SimpleQueue, Full, Empty
from typing import Tuple

class can_data():
    def __init__(self, header: str, data: str) -> None:
        self.header = header
        self.data = data

class can_queue(SimpleQueue):
    def __init__(self, queue_size: int, shall_overwrite: bool) -> None:
        super().__init__()
        self.queue_size = queue_size
        self.shall_overwrite = shall_overwrite
        self.has_lost_data = False

    def __len__(self) -> int:
        return self.qsize()

    def add(self, element: can_data) -> bool:
        if self.qsize() >= self.queue_size:
            self.has_lost_data = True
            if self.shall_overwrite:
                self.get_nowait()  # Bloque si pas no_wait. Retourne une erreur si file vide mais ne devrait pas être le cas.
                self.put(element)  # Ne bloque jamais
            else:
                return False  # Sous forme d'exception: Full("can_queue: added to full queue")
        else:
            self.put(element)
        return True

    def pop(self) -> Tuple[can_data, bool]:
        try:
            return self.get_nowait(), True  # Bloque si pas no_wait.
        except Empty:
            return can_data("", ""), False

    def length(self) -> int:  # len(q) fonctionne aussi
        return self.qsize()

    def has_lost_data(self) -> bool:
        return self.has_lost_data

    def stop(self) -> None:
        pass  # Seulement là à des fins de compatibilité.

