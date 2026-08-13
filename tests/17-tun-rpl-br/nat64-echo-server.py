#!/usr/bin/env python3
"""IPv4 UDP and TCP echo servers used by the NAT64 end-to-end test.

Listens on 127.0.0.1 only.  Each datagram or connection is logged
(timestamp + protocol + peer + payload) so the shell driver can grep
for evidence that the NAT64 gateway translated traffic from the IPv6
mote into IPv4.
"""

import argparse
import os
import signal
import socket
import sys
import threading
import time


def log(fp, msg):
    fp.write("{:.3f} {}\n".format(time.time(), msg))
    fp.flush()


def udp_server(host, port, logfp, stop):
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    s.bind((host, port))
    s.settimeout(0.5)
    log(logfp, "UDP_LISTEN {}:{}".format(host, port))
    while not stop.is_set():
        try:
            data, addr = s.recvfrom(1024)
        except socket.timeout:
            continue
        log(logfp, "UDP_ECHO from={}:{} bytes={} payload={!r}".format(
            addr[0], addr[1], len(data), data[:64]))
        try:
            s.sendto(data, addr)
        except OSError as e:
            log(logfp, "UDP_SEND_ERR {}".format(e))
    s.close()


def tcp_handle(conn, peer, logfp):
    log(logfp, "TCP_ACCEPT from={}:{}".format(peer[0], peer[1]))
    conn.settimeout(10.0)
    try:
        # Read a small chunk and echo it back.
        data = conn.recv(1024)
        if not data:
            log(logfp, "TCP_EMPTY from={}:{}".format(peer[0], peer[1]))
            return
        log(logfp, "TCP_ECHO from={}:{} bytes={} payload={!r}".format(
            peer[0], peer[1], len(data), data[:64]))
        conn.sendall(data)
    except socket.timeout:
        log(logfp, "TCP_TIMEOUT from={}:{}".format(peer[0], peer[1]))
    except OSError as e:
        log(logfp, "TCP_ERR from={}:{} {}".format(peer[0], peer[1], e))
    finally:
        try:
            conn.shutdown(socket.SHUT_WR)
        except OSError:
            pass
        conn.close()


def tcp_server(host, port, logfp, stop):
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    s.bind((host, port))
    s.listen(8)
    s.settimeout(0.5)
    log(logfp, "TCP_LISTEN {}:{}".format(host, port))
    while not stop.is_set():
        try:
            conn, peer = s.accept()
        except socket.timeout:
            continue
        t = threading.Thread(target=tcp_handle, args=(conn, peer, logfp),
                             daemon=True)
        t.start()
    s.close()


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--host", default="127.0.0.1")
    ap.add_argument("--udp-port", type=int, default=5557)
    ap.add_argument("--tcp-port", type=int, default=5558)
    ap.add_argument("--log", required=True)
    ap.add_argument("--pidfile")
    args = ap.parse_args()

    if args.pidfile:
        with open(args.pidfile, "w") as f:
            f.write(str(os.getpid()))

    logfp = open(args.log, "w", buffering=1)
    log(logfp, "PID {}".format(os.getpid()))

    stop = threading.Event()

    def handle_signal(signum, frame):
        stop.set()

    signal.signal(signal.SIGTERM, handle_signal)
    signal.signal(signal.SIGINT, handle_signal)

    threads = [
        threading.Thread(target=udp_server,
                         args=(args.host, args.udp_port, logfp, stop),
                         daemon=True),
        threading.Thread(target=tcp_server,
                         args=(args.host, args.tcp_port, logfp, stop),
                         daemon=True),
    ]
    for t in threads:
        t.start()

    while not stop.is_set():
        time.sleep(0.5)

    log(logfp, "STOP")
    logfp.close()
    return 0


if __name__ == "__main__":
    sys.exit(main())
