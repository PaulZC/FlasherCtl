#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
Base interface to the I/O stack running on a CTA Flasher (Arduino Zero).
Subsystems should be derived from the IOStack class.
"""

import collections
import random
import socket
import struct
import sys


default_port = 512
default_timeout = 0.2
default_retries = 3
default_verbosity = 0
default_max_packet_size = 256

SYS_IOSTACK = 0

class Command(object):
    """I/O stack command codes."""
    CMD_READ_REG = 0
    CMD_WRITE_REG = 1
    CMD_PING = 2
    CMD_REPORT_ERR = 255


class Register(object):
    """I/O stack registers."""
    REG_ETHERNET_CFG = 0


class Status(object):
    """I/O stack error codes."""
    ERR_OKAY = 0
    ERR_UNKNOWN_SUBSYSTEM = 1
    ERR_UNKNOWN_COMMAND = 2
    ERR_INVALID_SIZE = 3
    ERR_INVALID_REGISTER = 4
    ERR_INVALID_MAC = 5
    ERR_UNHANDLED_ERROR = 6


# Generate lookup maps
for c in Command, Register, Status:
    c.lookup = {v: k for (k, v) in c.__dict__.iteritems()
                if not k.startswith('__')}


class Error(Exception):
    """Base exception in case we need extended functionality in the future."""
    pass


class TimeoutError(Error):
    """Timeout occured while waiting for a response."""
    pass


class ResponseError(Error):
    """Received a malformed response."""
    pass


class RequestError(Error):
    """Reply indicated an invalid request."""
    pass


Response = collections.namedtuple("Response",
                                  "id subsystem_id response_code "
                                  "payload")

SubsystemResponse = collections.namedtuple("SubsystemResponse",
                                           "response_code payload")


class IOStack(object):
    def __init__(self, ip, port=default_port, timeout=default_timeout,
                 max_retries=default_retries, verbosity=default_verbosity,
                 max_packet_size=default_max_packet_size,
                 interface_ip=None):
        """Connects to an I/O stack with the given address.

        Parameters
        ----------
        ip : string
            Destination address.
        port : int, optional
            Destination port (default: 512).
        timeout : float, optional
            Response timeout in seconds (default: 200 ms).
        max_retries : int, optional
            Maximum number of retries after a timeout (default: 3).
        verbosity : int, optional
            Verbosity level (default: 0, silent)
        max_packet_size : int, optional
            Expected maximum size of replies (default: 256 Bytes).
        interface_ip : str, optional
            IP address of local interface (default: let OS choose).
        """
        self.verbosity = verbosity
        self.max_packet_size = max_packet_size
        self.request_id = random.randint(0, 65535)
        self.max_retries = max_retries

        # Connect
        self.cs = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

        if interface_ip is not None:
            # Bind to a specific interface
            for local_port in xrange(1025, 65535):
                try:
                    self.cs.bind((interface_ip, local_port))
                    if self.verbosity:
                        print >> sys.stderr, "IOStack: bound to %s:%i" % (interface_ip, local_port)
                    break
                except:
                    pass
            else:
                raise RuntimeError("error: no free UDP port on interface with IP %s" % interface_ip)

            self.cs.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            self.cs.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)

        self.cs.connect((ip, port))
        self.cs.settimeout(timeout)
        if self.verbosity:
            print >> sys.stderr, "IOStack: opened connection to %s:%i, %.0f ms timeout" % (ip, port, 1000 * timeout)

    def multi_request(self, subsystem_id, request_code, payload, broadcast=False,
                      max_retries=None):
        """
        Send a request and yield one or more replies.

        Parameters
        ----------
        subsystem_id : int
            Identifier of the target subsystem.
        request_code : int
            Identifier of the command to execute.
        payload : bytearray or string
            Payload to append to the request.
        broadcast : bool, optional
            If true, wait for multiple answers (default: False).
        max_retries : int, optional
            Maximum number of retries after a timeout (None: use default
            value).

        Yields
        ------
        Response
            Responses with raw payload.
        """
        # Pack and transmit request
        self.request_id = (self.request_id + 1) % 65536
        request = struct.pack("<HBH", self.request_id, subsystem_id,
                              request_code) + payload

        self.cs.send(request)

        # Receive response
        trials_left = self.max_retries if max_retries is None else int(max_retries)
        while True:
            try:
                reply = self.cs.recv(self.max_packet_size)
            except socket.timeout:
                if trials_left > 0:  # Retry
                    trials_left -= 1
                    self.cs.send(request)
                    continue

                raise TimeoutError()

            if len(reply) < 5:
                raise ResponseError("response too short")

            header, payload = reply[:5], reply[5:]
            response = Response(*struct.unpack("<HBH", header),
                                payload=payload)

            if response.id != self.request_id:
                if self.verbosity:
                    print >> sys.stderr, "skipping packet %s" % str(response)

                continue

            # Perform sanity checks
            if response.subsystem_id == 0:
                if response.response_code not in Command.lookup:
                    raise ResponseError("invalid response code %i" % response.response_code)

                if response.response_code == Command.CMD_REPORT_ERR:
                    if len(response.payload) != 2:
                        raise ResponseError("invalid error size %i" % len(response.payload))

                    error_code, = struct.unpack("<H", response.payload)
                    if error_code not in Status.lookup:
                        raise ResponseError("invalid error code %i" % error_code)

                    raise RequestError(Status.lookup[error_code])

            if response.subsystem_id != subsystem_id:
                raise ResponseError("invalid subsystem id %i" % response.subsystem_id)

            yield response

            if not broadcast:
                break

    def request(self, subsystem_id, request_code, payload, max_retries=None):
        """Send a request and return the response.

        Parameters
        ----------
        subsystem_id : int
            Identifier of the target subsystem.
        request_code : int
            Identifier of the command to execute.
        payload : bytearray or string
            Payload to append to the request.
        max_retries : int, optional
            Maximum number of retries after a timeout (None: use default
            value).

        Returns
        -------
        Response
            Response with raw payload.
        """
        return list(self.multi_request(subsystem_id, request_code, payload,
                    False, max_retries=max_retries))[0]

    def multiread_register(self, register, type_code="<H", max_retries=None):
        """Read from a register from multiple devices and yield their contents.

        Parameters
        ----------
        register : int
            The register to read from.
        type_code : str
            The format of the response payload (see the documentation of Python's
            struct module).
        max_retries : int, optional
            Maximum number of retries after a timeout (None: use default
            value).

        Yields
        ------
        SubsystemResponse
            Response with payload decoded according to the type_code parameter.
        """
        payload = struct.pack("<H", register)

        for response in self.multi_request(SYS_IOSTACK, Command.CMD_READ_REG,
                                           payload, broadcast=True,
                                           max_retries=max_retries):
            yield SubsystemResponse(response.response_code,
                                    struct.unpack(type_code, response.payload))

    def read_register(self, register, type_code="<H", max_retries=None):
        """Reads from a register and returns its content.

        Parameters
        ----------
        register : int
            The register to read from.
        type_code : string
            The format of the response payload (see the documentation of Python's
            struct module).
        max_retries : int, optional
            Maximum number of retries after a timeout (None: use default
            value).

        Returns
        -------
        SubsystemResponse
            Response with payload decoded according to the type_code parameter.
        """
        payload = struct.pack("<H", register)

        response = self.request(SYS_IOSTACK, Command.CMD_READ_REG, payload,
                                max_retries=max_retries)

        return SubsystemResponse(response.response_code,
                                 struct.unpack(type_code, response.payload))

    def write_register(self, register, value, type_code="<H", max_retries=None):
        """Writes to a register.

        Parameters
        ----------
        register : int
            The register to write to.
        value : int, bytearray or str
            The value to write to the register. If typecode is not None, the
            value is first encoded with struct.pack using the given format.
        type_code : str
            The format of value (see the documentation of Python's struct
            module). If None, value is assumed to be a bytearray or string
            which will not be encoded.
        max_retries : int, optional
            Maximum number of retries after a timeout (None: use default
            value).
        """
        payload = struct.pack("<H", register)
        if type_code:  # pack value according to type code
            payload += struct.pack(type_code, *value)
        else:  # transparently pass value
            payload += value

        response = self.request(SYS_IOSTACK, Command.CMD_WRITE_REG, payload,
                                max_retries=max_retries)

        return response.response_code

    def ping(self, payload=None):
        """Probes the connection to the device by sending a random payload."""
        header_size = 5
        if payload is None:
            payload = bytearray(random.randint(0, 255)
                                for i in xrange(64 - header_size))
        elif len(payload) > self.max_packet_size - header_size:
            payload = payload[:self.max_packet_size - header_size]

        response = self.request(SYS_IOSTACK, Command.CMD_PING, payload, max_retries=0)
        if response.payload != payload:
            raise ResponseError("payload mismatch")
