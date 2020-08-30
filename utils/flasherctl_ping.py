#! /usr/bin/env python
# -*- coding: utf-8 -*-

import argparse
import random
import time

import iostack


def std(x):
    from math import sqrt

    if len(x) < 2:
        return float("inf")
    else:
        l = float(len(x))
        return sqrt((sum(ix**2 for ix in x) / l - (sum(x) / l)**2))


if __name__ == '__main__':
    # Parse command-line arguments
    parser = argparse.ArgumentParser(description='ping slow control devices')
    parser.add_argument('ip', type=str, help="IP address")
    parser.add_argument('-p', metavar='port', type=int, default=512,
                        help='query port (default: 512)')
    parser.add_argument('-s', metavar='size', type=int, default=63,
                        help='payload size (default: 63)')
    parser.add_argument('-i', metavar='wait', type=float, default=1.0,
                        help='wait time between packets (default: 1 second)')

    args = parser.parse_args()
    if args.s < 0:
        args.s = 0
    elif args.s > 251:
        args.s = 251
    if args.i < 0.0:
        args.i = 0.0

    io = iostack.IOStack(args.ip, args.p, verbosity=0)

    try:
        nsent, nrecvd = 0, 0
        dts = []
        while True:
            payload = bytearray(random.randint(0, 255) for i in range(args.s))
            t0 = time.time()

            nsent += 1
            try:
                io.ping(payload)
            except iostack.TimeoutError:
                print("timed out")
                continue

            dt = 1000.0 * (time.time() - t0)
            nrecvd += 1
            dts.append(dt)

            print("%i byte%s from %s: time=%.3f ms" % (args.s, "" if args.s == 1 else "s", args.ip, dt))
            time.sleep(max(args.i - dt / 1000.0, 0.0))
    except KeyboardInterrupt:
        print
        if nsent > 0 and len(dts) > 0:
            print("--- %s ping statistics ---" % args.ip)
            print("%i packet%s transmitted, %i packet%s received, %.1f%% packet loss" % (nsent, "" if nsent == 1 else "s", nrecvd, "" if nrecvd == 1 else "s", 100 * (nsent - nrecvd) / nsent))
            print("round-trip min/avg/max/stddev = %.3f/%.3f/%.3f/%.3f ms" % (min(dts), sum(dts) / len(dts), max(dts), std(dts)))
