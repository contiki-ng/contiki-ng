#!/usr/bin/env python3
"""Minimal IPv4 DNS server for the ip64 TAP test.

Answers one name with a fixed A record, and logs the query type of every
request.  The query type is the point of the exercise: the node asks for
AAAA, so an A query arriving here means the DNS64 part of ip64 rewrote the
question on the way out.
"""

import argparse
import os
import signal
import socket
import struct
import sys
import time

TYPE_A = 1
TYPE_AAAA = 28


def log(fp, msg):
    fp.write("{:.3f} {}\n".format(time.time(), msg))
    fp.flush()


def parse_question(data):
    """Return (name, qtype, end_offset) for the first question."""
    labels = []
    offset = 12
    while offset < len(data):
        length = data[offset]
        offset += 1
        if length == 0:
            break
        labels.append(data[offset:offset + length].decode("ascii", "replace"))
        offset += length
    if offset + 4 > len(data):
        raise ValueError("truncated question")
    qtype, _ = struct.unpack("!HH", data[offset:offset + 4])
    return ".".join(labels), qtype, offset + 4


def build_response(query, question_end, answer_addr):
    """Echo the question and append one A record for answer_addr."""
    qid = query[0:2]
    header = qid + struct.pack("!HHHHH", 0x8180, 1, 1, 0, 0)
    question = query[12:question_end]
    answer = struct.pack("!HHHIH", 0xc00c, TYPE_A, 1, 3600, 4)
    answer += socket.inet_aton(answer_addr)
    return header + question + answer


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--host", default="0.0.0.0")
    parser.add_argument("--port", type=int, default=53)
    parser.add_argument("--name", required=True)
    parser.add_argument("--answer", required=True)
    parser.add_argument("--log", required=True)
    parser.add_argument("--pidfile")
    args = parser.parse_args()

    logfp = open(args.log, "a")
    if args.pidfile:
        with open(args.pidfile, "w") as fp:
            fp.write("{}\n".format(os.getpid()))

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.bind((args.host, args.port))
    log(logfp, "DNS_LISTEN {}:{}".format(args.host, args.port))

    def stop(signum, frame):
        log(logfp, "DNS_STOP")
        sys.exit(0)

    signal.signal(signal.SIGTERM, stop)
    signal.signal(signal.SIGINT, stop)

    while True:
        data, addr = sock.recvfrom(1024)
        try:
            name, qtype, question_end = parse_question(data)
        except (IndexError, ValueError) as e:
            log(logfp, "DNS_BAD_QUERY from={} err={}".format(addr[0], e))
            continue

        log(logfp, "DNS_QUERY from={} name={} qtype={}".format(
            addr[0], name, qtype))

        if qtype == TYPE_A and name == args.name:
            sock.sendto(build_response(data, question_end, args.answer), addr)
            log(logfp, "DNS_ANSWER name={} a={}".format(name, args.answer))
        else:
            # Anything else is left unanswered on purpose: an AAAA query here
            # would mean DNS64 did not rewrite the question.
            log(logfp, "DNS_IGNORED name={} qtype={}".format(name, qtype))


if __name__ == "__main__":
    main()
