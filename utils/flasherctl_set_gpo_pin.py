#!/usr/bin/env python
# -*- coding: utf-8 -*-

import argparse

import iostack
import flasherctl

import time

if __name__ == '__main__':
    # Parse command-line arguments
    parser = argparse.ArgumentParser(
        description='Configure the selected GPO pin on the selected flasher timing board')
    parser.add_argument('ip', type=str, help="IP address")
    parser.add_argument('board', metavar='board', type=int,
                        help='board no. (0:3)')
    parser.add_argument('pin', metavar='pin', type=int,
                        help='pin no. (0:1)')
    parser.add_argument('on_off', metavar='on_off', type=int,
                        help='0 (off) : 1 (on)')
    parser.add_argument('-p', metavar='port', type=int,
                        default=iostack.default_port,
                        help='port (default: %i)' % iostack.default_port)

    args = parser.parse_args()

    # Create connection
    flasher = flasherctl.FlasherCtl(args.ip, args.p, verbosity=0)

    print "setting GPO pin %d on board %d to %d" % (args.pin, args.board, args.on_off)
    flasher._SET_GPO_PIN(args.board, args.pin, args.on_off)
