#!/usr/bin/env python
# -*- coding: utf-8 -*-

import argparse

import iostack
import flasherctl

if __name__ == '__main__':
    # Parse command-line arguments
    parser = argparse.ArgumentParser(
        description='remote control of flasher LED_BUILTIN')
    parser.add_argument('ip', type=str, help="IP address")
    parser.add_argument('on_off', metavar='on_off', type=int,
                        help='1 (on) or 0 (off)')
    parser.add_argument('-p', metavar='port', type=int,
                        default=iostack.default_port,
                        help='port (default: %i)' % iostack.default_port)

    args = parser.parse_args()

    # Create connection
    flasher = flasherctl.FlasherCtl(args.ip, args.p, verbosity=0)

    print "setting LED_BUILTIN to", args.on_off
    flasher._LED_BUILTIN(args.on_off)
