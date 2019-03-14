#!/usr/bin/env python
# -*- coding: utf-8 -*-

import argparse

import iostack
import flasherctl

import time

if __name__ == '__main__':
    # Parse command-line arguments
    parser = argparse.ArgumentParser(
        description='read temperature from selected flasher timing board')
    parser.add_argument('ip', type=str, help="IP address")
    parser.add_argument('board', metavar='board', type=int,
                        help='board no. (0:3)')
    parser.add_argument('-p', metavar='port', type=int,
                        default=iostack.default_port,
                        help='port (default: %i)' % iostack.default_port)

    args = parser.parse_args()

    # Create connection
    flasher = flasherctl.FlasherCtl(args.ip, args.p, verbosity=0)

    print "reading temperature from board ", args.board
    flasher._START_TEMPERATURE(args.board)
    time.sleep(0.5)
    print "temperature is %f C" % flasher._READ_TEMPERATURE(args.board)
