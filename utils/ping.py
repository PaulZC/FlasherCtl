#! /usr/bin/env python
# -*- coding: utf-8 -*-

import iostack
import time
import random

def std(x):
    from math import sqrt

    if len(x) < 2:
        return float("inf")
    else:
        l = float(len(x))
        return sqrt((sum(ix**2 for ix in x) / l - (sum(x) / l)**2))


ip = '192.168.0.200' # flasher ip address
port = 512 # flasher port
length = 63 # ping length
interval = 1 # ping interval

io = iostack.IOStack(ip, port, verbosity=0)

try:
    nsent, nrecvd = 0, 0
    dts = []
    while True:
        payload = bytearray(random.randint(0, 255) for i in range(length))
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

        print ("%i byte%s from %s: time=%.3f ms" % (length, "" if length == 1 else "s", ip, dt))
        time.sleep(max(interval - dt / 1000.0, 0.0))
except KeyboardInterrupt:
    print
    if (nsent > 0) and (len(dts) > 0):
        print ("--- %s ping statistics ---" % ip)
        print ("%i packet%s transmitted, %i packet%s received, %.1f%% packet loss" % (nsent, "" if nsent == 1 else "s", nrecvd, "" if nrecvd == 1 else "s", 100 * (nsent - nrecvd) / nsent))
        print ("round-trip min/avg/max/stddev = %.3f/%.3f/%.3f/%.3f ms" % (min(dts), sum(dts) / len(dts), max(dts), std(dts)))
