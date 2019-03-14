#!/usr/bin/env python
# -*- coding: utf-8 -*-

import argparse

import iostack
import flasherctl

import time

if __name__ == '__main__':
    # Parse command-line arguments
    parser = argparse.ArgumentParser(
        description='Configure the ten LEDs on the selected flasher timing board')
    parser.add_argument('ip', type=str, help="IP address")
    parser.add_argument('board', metavar='board', type=int,
                        help='board no. (0:3)')
    parser.add_argument('ten_bit_string', metavar='ten_bit_string', type=str,
                        help='LED configuration bit string ("0000000000":"1111111111")')
    parser.add_argument('-p', metavar='port', type=int,
                        default=iostack.default_port,
                        help='port (default: %i)' % iostack.default_port)

    args = parser.parse_args()

    # Create connection
    flasher = flasherctl.FlasherCtl(args.ip, args.p, verbosity=0)

    print "configuring LEDs on board ", args.board
    flasher._SET_LEDS(args.board, int(args.ten_bit_string, 2))
