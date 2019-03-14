#!/usr/bin/env python
# -*- coding: utf-8 -*-

import argparse

import iostack
import flasherctl

import time

if __name__ == '__main__':
    # Parse command-line arguments
    parser = argparse.ArgumentParser(
        description='read the state of the TRIG signal on the selected board')
    parser.add_argument('ip', type=str, help="IP address")
    parser.add_argument('board', metavar='board', type=int,
                        help='board no. (0:3)')
    parser.add_argument('-p', metavar='port', type=int,
                        default=iostack.default_port,
                        help='port (default: %i)' % iostack.default_port)

    args = parser.parse_args()

    # Create connection
    flasher = flasherctl.FlasherCtl(args.ip, args.p, verbosity=0)

    print "reading TRIG on board", args.board
    print "TRIG signal is", flasher._READ_TRIG(args.board)
