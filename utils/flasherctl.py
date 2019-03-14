#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
Access to CTA Flasher.
Instantiate a FlasherCtl object and access flasher using its properties.
"""

import iostack
import struct
import time


# Subsystem ID for the flashers
# The iostack is "0"
# The Dynamixel servos are "1"
SYS_FLASHER = 2


class FlasherError:
    """Errors encountered during communication."""
    ERR_BVALUE = 1  # Binary / Boolean Value was not 1 (on) or 0 (off) (for LED_BUILTIN)
    ERR_FLASHER_ID = 2 # Flasher ID was not in the range 0:3
    ERR_TIMEDOUT = 128  # Communication with flasher timed out
    ERR_RX_CHECKSUM = 256  # Checksum error in flasher response
    ERR_MISMATCH = 512  # Received response from different flasher ID
    ERR_READ_ONLY = 1024  # Tried to write to a read-only register
    ERR_RX_FSM = 32768  # Bug in the receiving state machine


class FlasherCommand:
    """Command codes."""
    CMD_LED_BUILTIN = 0
    CMD_START_TEMPERATURE = 1
    CMD_READ_TEMPERATURE = 2
    CMD_INIT_FLASHER = 3
    CMD_SET_GPO_PIN = 4
    CMD_READ_SERIAL_NO = 5
    CMD_SET_LEDS = 6
    CMD_SET_PULSE_WIDTH = 7
    CMD_TEST_PULSE = 8
    CMD_READ_TRIG = 9
    CMD_REPORT_ERR = 65535


class FlasherCtl(iostack.IOStack):
    def __init__(self, ip, port=iostack.default_port,
                 timeout=iostack.default_timeout,
                 max_retries=iostack.default_retries,
                 verbosity=iostack.default_verbosity,
                 max_packet_size=iostack.default_max_packet_size,
                 interface_ip=None):
        """Connects to the flasher controller at the given address.

        Parameters
        ----------
        ip : string
            Destination address.
        port : int, optional
            Destination port (default: 512).
        timeout : float, optional
            Response timeout in seconds (default: 200 ms).
        verbosity : int, optional
            Verbosity level (default: 0, silent)
        max_packet_size : int, optional
            Expected maximum size of replies (default: 256 Bytes).
        interface_ip : str, optional
            IP address of local interface (default: let OS choose).
        """
        iostack.IOStack.__init__(self, ip=ip, port=port, timeout=timeout,
                                 max_retries=max_retries, verbosity=verbosity,
                                 max_packet_size=max_packet_size,
                                 interface_ip=interface_ip)


    def _LED_BUILTIN(self, value):
        """Configure flasher controller LED_BUILTIN.

        Parameters
        ----------
        value : byte
            State for LED_BUILTIN: 0 (off) or 1 (on)."""
        payload = struct.pack("<B", value)
        response = self.request(SYS_FLASHER, FlasherCommand.CMD_LED_BUILTIN,
                                payload)

        if response.response_code == FlasherCommand.CMD_LED_BUILTIN:
            return

        self._raise_error(response)

    def _START_TEMPERATURE(self, board):
        """Start an ADT7310TRZ temperature measurement on the selected timing board.
        Conversion takes 240msec to complete.

        Parameters
        ----------
        board : byte
            Selected timing board (0:3).
        """
        payload = struct.pack("<B", board)
        response = self.request(SYS_FLASHER, FlasherCommand.CMD_START_TEMPERATURE,
                                payload)

        if response.response_code == FlasherCommand.CMD_START_TEMPERATURE:
            return

        self._raise_error(response)

    def _READ_TEMPERATURE(self, board):
        """Read ADT7310TRZ temperature from selected timing board.

        Parameters
        ----------
        board : byte
            Selected timing board (0:3).

        Returns
        -------
        float
            The temperature reading.
        """
        payload = struct.pack("<B", board)
        response = self.request(SYS_FLASHER, FlasherCommand.CMD_READ_TEMPERATURE,
                                payload)

        if response.response_code == FlasherCommand.CMD_READ_TEMPERATURE:
            return struct.unpack("<f", response.payload)[0]

        self._raise_error(response)

    def _INIT_FLASHER(self, board):
        """Initialise the selected timing board I/O pins.

        Parameters
        ----------
        board : byte
            Selected timing board (0:3).
        """
        payload = struct.pack("<B", board)
        response = self.request(SYS_FLASHER, FlasherCommand.CMD_INIT_FLASHER,
                                payload)

        if response.response_code == FlasherCommand.CMD_INIT_FLASHER:
            return

        self._raise_error(response)

    def _SET_GPO_PIN(self, board, pin, on_off):
        """Set the selected GPO pin on or off on the selected timing board.

        Parameters
        ----------
        board : byte
            Selected timing board (0:3).
        pin : byte
            Selected pin (0:1).
        on_off : byte
            State for selected pin: 0 (off) or 1 (on).
        """
        payload = struct.pack("<BBB", board, pin, on_off)
        response = self.request(SYS_FLASHER, FlasherCommand.CMD_SET_GPO_PIN,
                                payload)

        if response.response_code == FlasherCommand.CMD_SET_GPO_PIN:
            return

        self._raise_error(response)

    def _SET_LEDS(self, board, leds):
        """Enable / disable the ten LEDs on the selected timing board.

        Parameters
        ----------
        board : byte
            Selected timing board (0:3).
        leds : word (16 bits)
            The 10 least significant bits enable/disable the LEDs.
            If bit 0 is '1', LED 1 is enabled.
            If bit 9 is '0', LED 10 is disabled.
        """
        payload = struct.pack("<BH", board, leds)
        response = self.request(SYS_FLASHER, FlasherCommand.CMD_SET_LEDS,
                                payload)

        if response.response_code == FlasherCommand.CMD_SET_LEDS:
            return

        self._raise_error(response)

    def _SET_PULSE_WIDTH(self, board, width):
        """Set the DS1023 delay (pulse width) on the selected timing board.

        Parameters
        ----------
        board : byte
            Selected timing board (0:3).
        width : byte
            The delay (pulse width) in 0.25ns increments.
        """
        payload = struct.pack("<BB", board, width)
        response = self.request(SYS_FLASHER, FlasherCommand.CMD_SET_PULSE_WIDTH,
                                payload)

        if response.response_code == FlasherCommand.CMD_SET_PULSE_WIDTH:
            return

        self._raise_error(response)

    def _TEST_PULSE(self, on_off):
        """Set the controller board test pulse line high or low.

        Parameters
        ----------
        on_off : byte
            State: 0 (off) or 1 (on).
        """
        payload = struct.pack("<B", on_off)
        response = self.request(SYS_FLASHER, FlasherCommand.CMD_TEST_PULSE,
                                payload)

        if response.response_code == FlasherCommand.CMD_TEST_PULSE:
            return

        self._raise_error(response)

    def _READ_TRIG(self, board):
        """Read the status of the TRIG signal from selected timing board.

        Parameters
        ----------
        board : byte
            Selected timing board (0:3).

        Returns
        -------
        byte
            TRIG status (0 or 1).
        """
        payload = struct.pack("<B", board)
        response = self.request(SYS_FLASHER, FlasherCommand.CMD_READ_TRIG,
                                payload)

        if response.response_code == FlasherCommand.CMD_READ_TRIG:
            return struct.unpack("<B", response.payload)[0]

        self._raise_error(response)

    def _READ_SERIAL_NO(self, board):
        """Read DS28CM00 serial number from selected timing board.

        Parameters
        ----------
        board : byte
            Selected timing board (0:3).

        Returns
        -------
        6 * bytes
            The 48-bit serial number in big endian format.
        """
        payload = struct.pack("<B", board)
        response = self.request(SYS_FLASHER, FlasherCommand.CMD_READ_SERIAL_NO,
                                payload)

        if response.response_code == FlasherCommand.CMD_READ_SERIAL_NO:
            return struct.unpack("6B", response.payload)

        self._raise_error(response)

    def _raise_error(self, response):
        """Raises an appropriate exception."""
        if response.response_code != FlasherCommand.CMD_REPORT_ERR:
            raise iostack.ResponseError("unknown response code %i"
                                        % response.response_code)

        if len(response.payload) != 2:
            raise iostack.ResponseError("invalid error size %i"
                                        % len(response.payload))

        error_code, = struct.unpack("<H", response.payload)
        if error_code not in FlasherError.lookup:
            raise iostack.ResponseError("unknown error code %i"
                                        % error_code)

        raise iostack.RequestError(FlasherError.lookup[error_code])


# Generate lookup maps
FlasherError.lookup = {v: k for (k, v) in FlasherError.__dict__.iteritems()
                if not k.startswith('__')}

if __name__ == '__main__':
    import sys

    if len(sys.argv) > 1: # If argv contains an IP number
        try:
            flasher = FlasherCtl(sys.argv[1])

            print "Toggling LED_BUILTIN"
            print "Ctrl-C to exit..."
            while True:
                flasher._LED_BUILTIN(1)
                time.sleep(1)
                flasher._LED_BUILTIN(0)
                time.sleep(1)
        except KeyboardInterrupt:
            pass

