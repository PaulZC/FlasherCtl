import flasherctl
import time

# Create connection
flasher = flasherctl.FlasherCtl('192.168.0.200', 512, verbosity=1)

try:
    print('Flashing with 64ns pulses')
    print('Crtl-C to stop')
    flasher._TEST_PULSE(0)
    print('Setting pulse width to 64 nsec')
    flasher._SET_PULSE_WIDTH(255)
    time.sleep(0.5)
    while True:
        #for CURRENT in [0,1]:
        #for CURRENT in [0,2]:
        #for CURRENT in [0,4]:
        #for CURRENT in [0,8]:
        for CURRENT in range(0, 16):
            print('Flashing with current setting %i' % CURRENT)
            flasher._SET_LED_CURRENT(CURRENT)
            flasher._TEST_PULSE(1)
            time.sleep(0.01)
            flasher._TEST_PULSE(0)
            time.sleep(0.01)
except KeyboardInterrupt:
    print('Bye!')
    pass
