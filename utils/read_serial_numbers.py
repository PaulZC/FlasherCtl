import flasherctl
import time

# Create connection
flasher = flasherctl.FlasherCtl('192.168.0.200', 512, verbosity=1)

try:
    print('Reading timing board serial numbers')
    print('Crtl-C to stop')
    while True:
        for board in range(0, 4):
            serial_no = ''.join(format(x, '02x') for x in flasher._READ_SERIAL_NO(board))
            print('Board %d serial number is 0x%s' % (board, serial_no))
            time.sleep(1)
except KeyboardInterrupt:
    print('Bye!')
    pass
