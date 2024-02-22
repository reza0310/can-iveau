#!/usr/bin/env python

"""
This example shows how sending a single message works.
"""

import can
import threading

def test(bus):
    print(bus.recv().data)

def send_one():
    """Sends a single message."""
      
    # this uses the default configuration (for example from the config file)
    # see https://python-can.readthedocs.io/en/stable/configuration.html
    bus = can.Bus(interface = "socketcan", bitrate = 500000, channel="can-iveau")
        # Using specific buses works similar:
        # bus = can.Bus(interface='socketcan', channel='vcan0', bitrate=250000)
        # bus = can.Bus(interface='pcan', channel='PCAN_USBBUS1', bitrate=250000)
        # bus = can.Bus(interface='ixxat', channel=0, bitrate=250000)
        # bus = can.Bus(interface='vector', app_name='CANalyzer', channel=0, bitrate=250000)
        # ...

    msg = can.Message(
        arbitration_id=0x1FFE30AA, data=[0, 25, 0, 1, 3, 1, 4, 1], is_extended_id=True
        )

    try:
        bus.send(msg)
        print(f"Message sent on {bus.channel_info}")
    except can.CanError:
        print("Message NOT sent")

    t = threading.Thread(target=test, args=(bus,))
    t.start()
    t.join()

    bus.shutdown()


if __name__ == "__main__":
    send_one()
