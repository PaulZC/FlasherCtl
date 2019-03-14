import flasherctl
import time

# Create connection
flasher = flasherctl.FlasherCtl('192.168.0.200', 512, verbosity=1)

try:
    print('Testing the LEDs with 64ns pulses')
    print('Crtl-C to stop')
    flasher._TEST_PULSE(0)
    for board in range(0, 4):
        print('Setting board %d pulse width to 64 nsec' % board)
        flasher._SET_PULSE_WIDTH(board, 255)
        time.sleep(0.5)
    while True:
        for LED in range(0, 10):
            print('Triggering LED %i' % (LED + 1))
            LED_bits = 1 << LED
            for board in range(0, 4):
                flasher._SET_LEDS(board, LED_bits)
            flasher._TEST_PULSE(1)
            time.sleep(0.1)
            for board in range(0, 1): #4):
                if (flasher._READ_TRIG(board) != 1): print('TRIG did not match on board %d!!' % board)
            flasher._TEST_PULSE(0)
            time.sleep(0.1)
            for board in range(0, 1): #4):
                if (flasher._READ_TRIG(board) != 0): print('TRIG did not match on board %d!!' % board)
            time.sleep(0.8)
except KeyboardInterrupt:
    print('Bye!')
    pass
