#!/usr/bin/env python
# -*- coding: utf-8 -*-

import argparse

import iostack
import flasherctl

import time

if __name__ == '__main__':
    # Parse command-line arguments
    parser = argparse.ArgumentParser(
        description='remote control of flasher TEST_PULSE')
    parser.add_argument('ip', type=str, help="IP address")
    parser.add_argument('on_off', metavar='on_off', type=int,
                        help='0 (low), 1 (high), 2 (pulse high then low)')
    parser.add_argument('-p', metavar='port', type=int,
                        default=iostack.default_port,
                        help='port (default: %i)' % iostack.default_port)

    args = parser.parse_args()

    # Create connection
    flasher = flasherctl.FlasherCtl(args.ip, args.p, verbosity=0)

    flasher._TEST_PULSE(args.on_off)
