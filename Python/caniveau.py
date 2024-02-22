import subprocess
import threading
import os
import can
from can_queue import can_queue, can_data
from typing import List, Tuple


def gather(receiver):  # Prends une instance de Caniveau et ne renvoie rien
    print(receiver.bus.recv().data)
    """while not receiver.finished.is_set():
        receiver.bus_lock.acquire()
        msg = b.recv()
        #msg = receiver.bus.recv(timeout=0.1)  # Bloquant si on retire le timeout
        #if msg:  # Quand on a un msg reçu
        if msg.is_rx:  # Si c'est pas nous qui l'avons envoyé
            data = []
            for i in range(8):
                data.append(int(msg.data&0xFF))
                msg.data >>= 8
            receiver.queue.add(can_data(msg.arbitration_id, data))"""
        #receiver.bus_lock.release()

class Caniveau():
    def __init__(self, bus: str, board_type: int, board_id: int, mailbox_size: int) -> None:
        self.bus = can.Bus(interface="socketcan", channel=bus, bitrate=500000)
        self.board_type = board_type
        self.board_id = board_id
        self.mailbox_size = mailbox_size
        self.bus_lock = threading.Lock()

        # Verification de la configuration
        if "not found" in subprocess.run(["candump", "-h"], capture_output=True, text=True).stdout:
            raise Exception("can-utils isn't installed !")
        if "not found" in subprocess.run(["ifconfig", "-h"], capture_output=True, text=True).stdout:
            raise Exception("net-tools isn't installed !")
        if "not found" in subprocess.run(["ifconfig", bus], capture_output=True, text=True).stdout:
            raise Exception(bus+" isn't set up properly !")

        # Mise en place des masques
        self.filters = []
        self.filters_numerotation = {}
        self.generate_filters()

        # Mise en place de la file de réception
        self.queue = can_queue(mailbox_size, True)

        # Mise en place de la réception
        self.finished = threading.Event()
        self.gatherer = threading.Thread(target=gather, args=(self,))
        #self.gatherer.start()

    def add_filter(self, filter_num: int, filter_id: int, filter_mask: int) -> bool:
        filter_id_str = hex(filter_id)
        filter_mask_str = hex(filter_mask)
        self.filters.append(filter_id_str+":"+filter_mask_str)
        self.filters_numerotation[filter_num] = len(self.filters)-1
        return True

    def generate_filters(self) -> bool:
        out =          self.add_filter(0, 0b00000000000000000000000000000,                0b00000000000011111000000000000)
        out =  out and self.add_filter(1, self.board_type << 12,                          0b00000000000011111111110000000)
        return out and self.add_filter(1, (self.board_type << 12) + (self.board_id << 7), 0b00000000000011111111110000000)

    def send_raw(self, header: int, data: List[int]) -> bool:
        self.bus.send(can.Message(arbitration_id=header, data=data, is_extended_id=True))
        return True

    def send_parsed(self, priority: int, message_type: int, message_id: int, board_type: int, board_id: int, tracking: int, data: List[int]) -> bool:
        header = 0

        header += priority
        header <<= 2
        header += message_type
        header <<= 2
        header += message_id
        header <<= 8
        header += board_type
        header <<= 5
        header += board_id
        header <<= 5
        header += tracking

        return self.send_raw(header, data)
    
    def send_parsed_checked(self, priority: int, message_type: int, message_id: int, board_type: int, board_id: int, tracking: int, data: List[int]) -> bool:
        if priority < 0 or priority > 3:
            return False
        if message_type < 0 or message_type > 3:
            return False
        if message_id < 0 or message_id > 255:
            return False
        if board_type < 0 or board_type > 31:
            return False
        if board_id < 0 or board_id > 31:
            return False
        if tracking < 0 or tracking > 127:
            return False
        return self.send_parsed(priority, message_type, message_id, board_type, board_id, tracking, data)

    def receive_raw(self) -> Tuple[can_data, bool]:
        return self.queue.pop()

    def receive_parsed(self) -> Tuple[int, int, int, int, int, int, List[int], bool]:
        data, check = self.receive_raw()
        if not check:
            return 0, 0, 0, 0, 0, 0, "", False
        
        header = int(data.header, 16)
        tracking = header&0x7F
        header >>= 7
        board_id = header&0x1F
        header >>= 5
        board_type = header&0x1F
        header >>= 5
        message_id = header&0xFF
        header >>= 8
        message_type = header&0x3
        header >>= 2
        priority = header&0x3

        return priority, message_type, message_id, board_type, board_id, tracking, data.data, True

    def stop(self) -> bool:
        self.finished.set()
        self.queue.stop()
        self.bus.shutdown()
        self.gatherer.join()
        return True

