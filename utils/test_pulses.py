import flasherctl
import time

# Create connection
flasher = flasherctl.FlasherCtl('192.168.0.200', 512, verbosity=1)

try:
    print('Toggling TEST_PULSE')
    print('Crtl-C to stop')
    while True:
        for lo_hi in range(0, 2):
            print('Setting TEST_PULSE to %i' % lo_hi)
            flasher._TEST_PULSE(lo_hi)
            for board in range(0, 4):
                print('Board %d TRIG is %i' % (board, flasher._READ_TRIG(board)))
                time.sleep(0.5)
except KeyboardInterrupt:
    print('Bye!')
    pass
