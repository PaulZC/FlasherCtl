import flasherctl
import time

# Create connection
flasher = flasherctl.FlasherCtl('192.168.0.200', 512, verbosity=1)

try:
    print('Reading flasher temperatures')
    print('Crtl-C to stop')
    while True:
        for board in range(0, 4):
            flasher._START_TEMPERATURE(board)
            time.sleep(0.5)
            print('Board %d temperature: %f C' % (board, flasher._READ_TEMPERATURE(board)))
            time.sleep(0.5)
except KeyboardInterrupt:
    print('Bye!')
    pass
