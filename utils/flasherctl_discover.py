#!/usr/bin/env python
# -*- coding: utf-8 -*-

import argparse
from socket import *
import netifaces

import iostack


def get_ip_interfaces():
    """Returns a list of broadcast-enabled IP interfaces."""
    result = []
    for interface, addresses in [(i, netifaces.ifaddresses(i)) for i in netifaces.interfaces()]:
        for inet_address in addresses.get(netifaces.AF_INET, []):
            if 'addr' in inet_address and 'broadcast' in inet_address:
                result.append((interface, inet_address['addr']))

    return result


def unpack_ethernet_configuration(reply):
    reply = reply[5:]
    gateway = list(reply[0:4])
    subnet = list(reply[4:8])
    mac = list(reply[8:14])
    ip = list(reply[14:18])

    return gateway, subnet, mac, ip


def pack_ethernet_configuration(gateway, subnet, mac, ip):
    import struct

    return struct.pack("B" * 18, *(gateway + subnet + mac + ip))


def prepare_socket(interface, interface_ip, timeout=1.0):
    cs = socket(AF_INET, SOCK_DGRAM)
    for local_port in range(1025, 65535):
        try:
            cs.bind((interface_ip, local_port))
            break
        except:
            pass
    else:
        raise RuntimeError("  error: no free UDP port on interface %s" % interface)

    cs.setsockopt(SOL_SOCKET, SO_REUSEADDR, 1)
    cs.setsockopt(SOL_SOCKET, SO_BROADCAST, 1)
    cs.settimeout(timeout)

    return cs


def query_interface(interface, interface_ip, port):
    import struct
    import random

    # Query current configuration
    cs = prepare_socket(interface, interface_ip)
    cs.sendto(struct.pack("<HBHH", random.randint(0, 65535), iostack.SYS_IOSTACK,
                          iostack.Command.CMD_READ_REG, iostack.Register.REG_ETHERNET_CFG),
              ("255.255.255.255", port))

    devices = []
    while True:
        try:
            gateway, subnet, mac, ip = unpack_ethernet_configuration(cs.recv(1024))
        except:
            break

        devices.append((gateway, subnet, mac, ip))
        print("  mac %s, ip %s, subnet %s, gateway %s" % (":".join("%02x" % m for m in mac),
                                                          ".".join(map(str, ip)),
                                                          ".".join(map(str, subnet)),
                                                          ".".join(map(str, gateway))))

    return devices


def configure_device(interface, interface_ip, port, old_mac, gateway, subnet, mac, ip):
    import struct
    import random

    cs = prepare_socket(interface, interface_ip)
    cs.sendto(struct.pack("<HBHH18B", random.randint(0, 65535), iostack.SYS_IOSTACK,
                          iostack.Command.CMD_WRITE_REG, iostack.Register.REG_ETHERNET_CFG,
                          *(gateway + subnet + mac + ip)),
              ("255.255.255.255", port))


def mkmac(s):
    try:
        tokens = s.split(":")
        assert len(tokens) == 6
        mac = [int(t, 16) for t in tokens]
        assert all(m >= 0 and m < 256 for m in mac)
    except:
        raise argparse.ArgumentTypeError("invalid format of mac address")

    return mac


def mkaddr(s):
    try:
        tokens = s.split(".")
        assert len(tokens) == 4
        addr = [int(t, 10) for t in tokens]
        assert all(a >= 0 and a < 256 for a in addr)
    except:
        raise argparse.ArgumentTypeError("invalid format of network address")

    return addr


if __name__ == '__main__':
    # Parse command-line arguments
    parser = argparse.ArgumentParser(description='discover and setup slow control devices')
    parser.add_argument('--port', metavar='n', type=int, default=512,
                        help='query port (default: 512)')
    parser.add_argument('--interface', metavar='name', type=str, default=None,
                        help='query interface (default: all)')
    parser.add_argument('mac', metavar='n:n:n:n:n:n', type=mkmac, default=None, nargs="?",
                        help='MAC address of target device (default: discover only)')
    parser.add_argument('--ip', metavar='n.n.n.n', type=mkaddr, default=None,
                        help='new IP address')
    parser.add_argument('--subnet', metavar='n.n.n.n', type=mkaddr, default=None,
                        help='new subnet mask')
    parser.add_argument('--gateway', metavar='n.n.n.n', type=mkaddr, default=None,
                        help='new gateway address')

    args = parser.parse_args()

    # Query all interfaces
    for interface, interface_ip in get_ip_interfaces():
        if args.interface and interface != args.interface:
            continue

        print("querying interface %s on interface_ip %s:" % (interface, interface_ip))

        result = query_interface(interface, interface_ip, args.port)

        if args.mac:
            for (gateway, subnet, mac, ip) in result:
                if list(mac) == args.mac:
                    print("  found device with mac %s" % ":".join("%02x" % x for x in args.mac))
                    old_ip = ip
                    if args.ip:
                        ip = args.ip
                    if args.subnet:
                        subnet = args.subnet
                    if args.gateway:
                        gateway = args.gateway

                    if args.ip or args.subnet or args.gateway:
                        print("  sending new configuration...")
                        configure_device(interface, interface_ip, args.port, old_ip,
                                         gateway, subnet, mac, ip)

                    break
