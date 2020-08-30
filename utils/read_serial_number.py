import flasherctl
import time

# Create connection
flasher = flasherctl.FlasherCtl('192.168.0.200', 512, verbosity=1)

try:
    print('Reading LED board serial number')
    print('Crtl-C to stop')
    while True:
        serial_no = ''.join(format(x, '02x') for x in flasher._READ_SERIAL_NO())
        print('Serial number is 0x%s' % serial_no)
        time.sleep(1)
except KeyboardInterrupt:
    print('Bye!')
    pass
