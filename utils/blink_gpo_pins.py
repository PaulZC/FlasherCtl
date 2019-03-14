import flasherctl
import time

# Create connection
flasher = flasherctl.FlasherCtl('192.168.0.200', 512, verbosity=1)

try:
    print('Blinking GPO pins on board 0')
    print('Crtl-C to stop')
    flasher._INIT_FLASHER(0) # Initialise the I/O pins on board 0
    while True:
        pass
        flasher._SET_GPO_PIN(0,0,0) # Board 0, GPO pin 0, off
        time.sleep(0.25)
        flasher._SET_GPO_PIN(0,1,0) # Board 0, GPO pin 1, off
        time.sleep(0.25)
        flasher._SET_GPO_PIN(0,0,1) # Board 0, GPO pin 0, on
        time.sleep(0.25)
        flasher._SET_GPO_PIN(0,1,1) # Board 0, GPO pin 1, on
        time.sleep(0.25)
except KeyboardInterrupt:
    print('Bye!')
    pass
