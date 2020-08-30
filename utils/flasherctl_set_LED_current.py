#!/usr/bin/env python
# -*- coding: utf-8 -*-

import argparse

import iostack
import flasherctl

import time

if __name__ == '__main__':
    # Parse command-line arguments
    parser = argparse.ArgumentParser(
        description='Configure the LED current')
    parser.add_argument('ip', type=str, help="IP address")
    parser.add_argument('current', metavar='current', type=int,
                        help='current (0:15)')
    parser.add_argument('-p', metavar='port', type=int,
                        default=iostack.default_port,
                        help='port (default: %i)' % iostack.default_port)

    args = parser.parse_args()

    # Create connection
    flasher = flasherctl.FlasherCtl(args.ip, args.p, verbosity=0)

    print("configuring LED current")
    flasher._SET_LED_CURRENT(args.current)
