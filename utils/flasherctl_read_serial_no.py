#!/usr/bin/env python
# -*- coding: utf-8 -*-

import argparse

import iostack
import flasherctl

import time

if __name__ == '__main__':
    # Parse command-line arguments
    parser = argparse.ArgumentParser(
        description='read the serial number from the selected board')
    parser.add_argument('ip', type=str, help="IP address")
    parser.add_argument('-p', metavar='port', type=int,
                        default=iostack.default_port,
                        help='port (default: %i)' % iostack.default_port)

    args = parser.parse_args()

    # Create connection
    flasher = flasherctl.FlasherCtl(args.ip, args.p, verbosity=0)

    serial_no = ''.join(format(x, '02x') for x in flasher._READ_SERIAL_NO(args.board))
    print('Board %d serial number is 0x%s' % (args.board, serial_no))
