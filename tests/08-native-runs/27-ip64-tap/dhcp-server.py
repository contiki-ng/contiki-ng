#!/usr/bin/env python3
"""Minimal IPv4 DHCP server for the ip64 TAP test.

Serves one fixed lease to any client that asks, and logs every message it
sees, so the test can tell how far the ip64 DHCP client got.  The client sets
the BOOTP broadcast flag, so replies go to 255.255.255.255 and no ARP is
needed before the lease exists.
"""

import argparse
import os
import signal
import socket
import struct
import sys
import time

DHCPDISCOVER = 1
DHCPOFFER = 2
DHCPREQUEST = 3
DHCPACK = 5

OPT_SUBNET_MASK = 1
OPT_ROUTER = 3
OPT_DNS_SERVER = 6
OPT_LEASE_TIME = 51
OPT_MSG_TYPE = 53
OPT_SERVER_ID = 54
OPT_END = 255

MAGIC_COOKIE = bytes([99, 130, 83, 99])
BOOTP_FIXED_LEN = 236

# SO_BINDTODEVICE is not exposed by the socket module on every Python build.
SO_BINDTODEVICE = getattr(socket, "SO_BINDTODEVICE", 25)


def log(fp, msg):
    fp.write("{:.3f} {}\n".format(time.time(), msg))
    fp.flush()


def parse_options(data):
    """Return {code: value} for the options following the magic cookie."""
    options = {}
    if len(data) < BOOTP_FIXED_LEN + 4:
        return options
    if data[BOOTP_FIXED_LEN:BOOTP_FIXED_LEN + 4] != MAGIC_COOKIE:
        return options

    offset = BOOTP_FIXED_LEN + 4
    while offset < len(data):
        code = data[offset]
        if code == OPT_END:
            break
        if code == 0:            # Padding.
            offset += 1
            continue
        if offset + 2 > len(data):
            break
        length = data[offset + 1]
        if offset + 2 + length > len(data):
            break
        options[code] = data[offset + 2:offset + 2 + length]
        offset += 2 + length
    return options


def build_reply(query, msg_type, offer, server, netmask, router):
    """Build an OFFER or an ACK for the client that sent query."""
    xid = query[4:8]
    chaddr = query[28:44]

    reply = struct.pack("!BBBB", 2, 1, 6, 0)      # op, htype, hlen, hops
    reply += xid
    reply += struct.pack("!HH", 0, 0x8000)        # secs, broadcast flag
    reply += socket.inet_aton("0.0.0.0")          # ciaddr
    reply += socket.inet_aton(offer)              # yiaddr
    reply += socket.inet_aton(server)             # siaddr
    reply += socket.inet_aton("0.0.0.0")          # giaddr
    reply += chaddr
    reply += bytes(64 + 128)                      # sname, file
    reply += MAGIC_COOKIE

    reply += bytes([OPT_MSG_TYPE, 1, msg_type])
    reply += bytes([OPT_SERVER_ID, 4]) + socket.inet_aton(server)
    reply += bytes([OPT_LEASE_TIME, 4]) + struct.pack("!I", 3600)
    reply += bytes([OPT_SUBNET_MASK, 4]) + socket.inet_aton(netmask)
    reply += bytes([OPT_ROUTER, 4]) + socket.inet_aton(router)
    reply += bytes([OPT_DNS_SERVER, 4]) + socket.inet_aton(server)
    reply += bytes([OPT_END])

    return reply


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--device", required=True,
                        help="interface to serve, e.g. tap0")
    parser.add_argument("--offer", required=True, help="address to lease")
    parser.add_argument("--server", required=True, help="our own address")
    parser.add_argument("--netmask", default="255.255.255.0")
    parser.add_argument("--router", required=True)
    parser.add_argument("--log", required=True)
    parser.add_argument("--pidfile")
    args = parser.parse_args()

    logfp = open(args.log, "a")
    if args.pidfile:
        with open(args.pidfile, "w") as fp:
            fp.write("{}\n".format(os.getpid()))

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
    # Bound to the TAP device, so that serving 0.0.0.0:67, which is what it
    # takes to receive a broadcast, cannot disturb the rest of the machine.
    sock.setsockopt(socket.SOL_SOCKET, SO_BINDTODEVICE,
                    args.device.encode() + b"\0")
    sock.bind(("", 67))
    log(logfp, "DHCP_LISTEN device={} offer={}".format(args.device,
                                                       args.offer))

    def stop(signum, frame):
        log(logfp, "DHCP_STOP")
        sys.exit(0)

    signal.signal(signal.SIGTERM, stop)
    signal.signal(signal.SIGINT, stop)

    while True:
        data, addr = sock.recvfrom(1024)
        if len(data) < BOOTP_FIXED_LEN:
            log(logfp, "DHCP_SHORT_MSG bytes={}".format(len(data)))
            continue

        options = parse_options(data)
        # Option 53 can arrive with no value at all, in which case there is
        # no message type to act on.
        msg_type_value = options.get(OPT_MSG_TYPE, b"")
        msg_type = msg_type_value[0] if msg_type_value else 0
        chaddr = data[28:34].hex(":")

        if msg_type == DHCPDISCOVER:
            log(logfp, "DHCP_DISCOVER chaddr={}".format(chaddr))
            reply = build_reply(data, DHCPOFFER, args.offer, args.server,
                                args.netmask, args.router)
            sock.sendto(reply, ("255.255.255.255", 68))
            log(logfp, "DHCP_OFFER ip={} chaddr={}".format(args.offer,
                                                           chaddr))
        elif msg_type == DHCPREQUEST:
            log(logfp, "DHCP_REQUEST chaddr={}".format(chaddr))
            reply = build_reply(data, DHCPACK, args.offer, args.server,
                                args.netmask, args.router)
            sock.sendto(reply, ("255.255.255.255", 68))
            log(logfp, "DHCP_ACK ip={} chaddr={}".format(args.offer, chaddr))
        else:
            log(logfp, "DHCP_OTHER type={} chaddr={}".format(msg_type,
                                                             chaddr))


if __name__ == "__main__":
    main()
