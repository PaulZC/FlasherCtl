import flasherctl
import time

# Create connection
flasher = flasherctl.FlasherCtl('192.168.0.200', 512, verbosity=1)

try:
    print('Flashing LED_BUILTIN')
    print('Crtl-C to stop')
    while True:
        flasher._LED_BUILTIN(1)
        time.sleep(1)
        flasher._LED_BUILTIN(0)
        time.sleep(1)
except KeyboardInterrupt:
    print('Bye!')
    pass


