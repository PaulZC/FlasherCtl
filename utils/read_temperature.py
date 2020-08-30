import flasherctl
import time

# Create connection
flasher = flasherctl.FlasherCtl('192.168.0.200', 512, verbosity=1)

try:
    print('Reading flasher temperature')
    print('Crtl-C to stop')
    while True:
        flasher._START_TEMPERATURE()
        time.sleep(0.5)
        print('Temperature: %f C' % (flasher._READ_TEMPERATURE()))
        time.sleep(0.5)
except KeyboardInterrupt:
    print('Bye!')
    pass
