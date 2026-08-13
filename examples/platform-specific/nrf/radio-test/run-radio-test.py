#!/usr/bin/env python3
#
# Copyright (c) 2026, RISE Research Institutes of Sweden AB
# All rights reserved.
#
# Author: Joakim Eriksson <joakim.eriksson@ri.se>
#
# SPDX-License-Identifier: BSD-3-Clause

"""Automate the nRF54L15 MAC-level radio test over two serial consoles.

This script expects both boards to already run the `radio-test` firmware.
It talks to the Contiki shell over UART, configures both nodes, runs a
matrix of one-way tests, and prints a summary.

It uses only Python's standard library. No pyserial dependency is required.
"""

from __future__ import annotations

import argparse
import glob
import json
import os
import re
import select
import sys
import termios
import time
from dataclasses import dataclass, asdict
from pathlib import Path
from typing import Dict, Iterable, List, Optional


PROMPT_RE = re.compile(r"#(?:[0-9a-f]{4}\.){3}[0-9a-f]{4}> ?")
RTSTAT_RE = re.compile(r"^RTSTAT\b.*$", re.MULTILINE)
DEFAULT_PORT_GLOBS = ("/dev/tty.usbmodem*", "/dev/ttyACM*")


def parse_csv_ints(value: str) -> List[int]:
    return [int(part.strip(), 0) for part in value.split(",") if part.strip()]


def unique_ordered(values: Iterable[str]) -> List[str]:
    seen = set()
    result: List[str] = []
    for value in values:
        if value not in seen:
            seen.add(value)
            result.append(value)
    return result


def configure_serial(fd: int, baud: int = termios.B115200) -> None:
    attrs = termios.tcgetattr(fd)

    attrs[0] &= ~(
        termios.IGNBRK
        | termios.BRKINT
        | termios.PARMRK
        | termios.ISTRIP
        | termios.INLCR
        | termios.IGNCR
        | termios.ICRNL
        | termios.IXON
    )
    attrs[1] &= ~termios.OPOST
    attrs[2] &= ~(termios.CSIZE | termios.PARENB | termios.PARODD)
    attrs[2] |= termios.CS8 | termios.CLOCAL | termios.CREAD
    attrs[3] &= ~(termios.ICANON | termios.ECHO | termios.ECHOE | termios.ISIG)
    attrs[4] = baud
    attrs[5] = baud
    attrs[6][termios.VMIN] = 0
    attrs[6][termios.VTIME] = 0

    termios.tcsetattr(fd, termios.TCSANOW, attrs)


class SerialShell:
    def __init__(self, port: str, label: Optional[str] = None, log_path: Optional[Path] = None):
        self.port = port
        self.label = label or port
        self.log_path = log_path
        self.fd = os.open(port, os.O_RDWR | os.O_NOCTTY | os.O_NONBLOCK)
        configure_serial(self.fd)
        self._buffer = ""
        self._log = None

        if log_path is not None:
            log_path.parent.mkdir(parents=True, exist_ok=True)
            self._log = log_path.open("w", encoding="utf-8", buffering=1)

    def close(self) -> None:
        if self._log is not None:
            self._log.close()
        os.close(self.fd)

    def _log_line(self, prefix: str, text: str) -> None:
        if self._log is None or text == "":
            return

        timestamp = time.strftime("%Y-%m-%d %H:%M:%S")
        for line in text.splitlines(True):
            if line.endswith("\n"):
                self._log.write(f"[{timestamp}] {prefix}{line}")
            else:
                self._log.write(f"[{timestamp}] {prefix}{line}\n")

    def _read_available(self, timeout: float) -> str:
        ready, _, _ = select.select([self.fd], [], [], timeout)
        if not ready:
            return ""

        data = os.read(self.fd, 4096)
        if not data:
            return ""

        text = data.decode("utf-8", errors="replace")
        self._log_line("RX ", text)
        self._buffer += text
        return text

    def _drain(self, idle_timeout: float = 0.1, max_time: float = 0.5) -> str:
        end = time.monotonic() + max_time
        collected = []
        while time.monotonic() < end:
            chunk = self._read_available(idle_timeout)
            if not chunk:
                break
            collected.append(chunk)
        return "".join(collected)

    def write_line(self, line: str) -> None:
        self._log_line("TX ", line + "\n")
        os.write(self.fd, line.encode("utf-8") + b"\n")

    def ensure_prompt(self, timeout: float = 8.0) -> None:
        deadline = time.monotonic() + timeout
        next_nudge = 0.0

        while time.monotonic() < deadline:
            now = time.monotonic()
            if now >= next_nudge:
                self.write_line("")
                next_nudge = now + 1.0

            self._read_available(0.2)
            if PROMPT_RE.search(self._buffer):
                self._buffer = ""
                self._drain()
                return

        raise TimeoutError(f"{self.label}: shell prompt not found on {self.port}")

    def run_command(self, command: str, timeout: float = 8.0, settle: float = 0.1) -> str:
        self.ensure_prompt()
        self._buffer = ""
        self.write_line(command)

        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            self._read_available(0.2)
            if PROMPT_RE.search(self._buffer):
                if settle > 0:
                    self._drain(idle_timeout=settle, max_time=settle)
                output = self._buffer
                self._buffer = ""
                return output

        raise TimeoutError(f"{self.label}: timed out waiting for command '{command}'")


def parse_rtstat(raw: str) -> Dict[str, str]:
    matches = RTSTAT_RE.findall(raw)
    if not matches:
        raise ValueError(f"Could not find RTSTAT line in output:\n{raw}")

    line = matches[-1].strip()
    result: Dict[str, str] = {}
    for token in line.split()[1:]:
        if "=" not in token:
            continue
        key, value = token.split("=", 1)
        result[key] = value

    return result


def to_int(value: str) -> int:
    return int(value, 0)


def stat_int(stats: Dict[str, str], key: str, default: int = 0) -> int:
    value = stats.get(key)
    return to_int(value) if value is not None else default


def command_ok(raw: str, expected: str) -> bool:
    return expected in raw


@dataclass
class Node:
    port: str
    shell: SerialShell
    local_mac: str
    label: str

    def run(self, command: str, timeout: float = 8.0) -> str:
        return self.shell.run_command(command, timeout=timeout)

    def status(self) -> Dict[str, str]:
        return parse_rtstat(self.run("radio-test status-brief"))


@dataclass
class TestCase:
    power: int
    channel: int
    length: int
    txmax: int

    @property
    def name(self) -> str:
        return f"p{self.power}_ch{self.channel}_len{self.length}_tx{self.txmax}"


@dataclass
class DirectionResult:
    case: str
    sender: str
    receiver: str
    target_packets: int
    sender_stats: Dict[str, str]
    receiver_stats: Dict[str, str]
    classification: str


def discover_candidate_ports(patterns: Iterable[str]) -> List[str]:
    ports: List[str] = []
    for pattern in patterns:
        ports.extend(glob.glob(pattern))
    return sorted(unique_ordered(ports))


def connect_node(port: str, label: Optional[str] = None, log_path: Optional[Path] = None) -> Node:
    shell = SerialShell(port, label=label, log_path=log_path)
    try:
        stats = parse_rtstat(shell.run_command("radio-test status-brief", timeout=10.0))
    except Exception:
        shell.close()
        raise

    local_mac = stats["local"]
    return Node(port=port, shell=shell, local_mac=local_mac, label=label or local_mac)


def autodiscover_nodes(patterns: Iterable[str], log_dir: Optional[Path] = None) -> List[Node]:
    nodes: List[Node] = []
    errors: List[str] = []

    for port in discover_candidate_ports(patterns):
        try:
            log_path = None
            if log_dir is not None:
                log_path = log_dir / f"{Path(port).name}.log"
            node = connect_node(port, log_path=log_path)
        except Exception as exc:  # pragma: no cover - error path is environment-specific
            errors.append(f"{port}: {exc}")
            continue
        nodes.append(node)

    if len(nodes) == 2:
        return nodes

    for node in nodes:
        node.shell.close()

    if len(nodes) > 2:
        raise RuntimeError(
            "Found more than two radio-test nodes. Use --ports to pick the pair:\n"
            + "\n".join(f"  {node.port} ({node.local_mac})" for node in nodes)
        )

    details = "\n".join(f"  {line}" for line in errors) if errors else "  (no candidates responded)"
    raise RuntimeError(
        "Could not auto-discover exactly two radio-test nodes.\n"
        "Make sure both boards are flashed with radio-test and no other program holds the serial ports.\n"
        f"Probe results:\n{details}"
    )


def configure_node(node: Node, case: TestCase, interval_ms: int, verbose: int) -> None:
    commands = [
        "radio-test stop",
        f"radio-test verbose {verbose}",
        f"radio-test power {case.power}",
        f"radio-test channel {case.channel}",
        f"radio-test len {case.length}",
        f"radio-test txmax {case.txmax}",
        f"radio-test interval {interval_ms}",
        "radio-test clear-target",
        "radio-test reset",
    ]

    for command in commands:
        node.run(command)


def wait_for_sender_count(node: Node, target_packets: int, interval_ms: int, timeout_s: float) -> Dict[str, str]:
    deadline = time.monotonic() + timeout_s
    poll_s = max(0.2, min(1.0, interval_ms / 1000.0 / 2.0))
    latest = node.status()

    while time.monotonic() < deadline:
        if stat_int(latest, "tx_started") >= target_packets:
            return latest
        time.sleep(poll_s)
        latest = node.status()

    raise TimeoutError(
        f"{node.label}: tx_started did not reach {target_packets}; last status: {latest}"
    )


def wait_until_idle(node: Node, timeout_s: float = 3.0) -> Dict[str, str]:
    deadline = time.monotonic() + timeout_s
    latest = node.status()

    while time.monotonic() < deadline:
        if (
            stat_int(latest, "running") == 0
            and stat_int(latest, "tx_busy") == 0
            and stat_int(latest, "pending") == 0
        ):
            return latest
        time.sleep(0.2)
        latest = node.status()

    return latest


def wait_for_receiver_settle(
    node: Node,
    settle_s: float = 0.5,
    timeout_s: float = 5.0,
) -> Dict[str, str]:
    deadline = time.monotonic() + timeout_s
    quiet_since: Optional[float] = None
    latest = node.status()
    last_rx_ok = stat_int(latest, "rx_ok")
    last_rx_seq = stat_int(latest, "last_rx_seq")

    while time.monotonic() < deadline:
        time.sleep(0.1)
        latest = node.status()
        rx_ok = stat_int(latest, "rx_ok")
        rx_seq = stat_int(latest, "last_rx_seq")

        if rx_ok != last_rx_ok or rx_seq != last_rx_seq:
            last_rx_ok = rx_ok
            last_rx_seq = rx_seq
            quiet_since = None
            continue

        if quiet_since is None:
            quiet_since = time.monotonic()
        elif time.monotonic() - quiet_since >= settle_s:
            return latest

    return latest


def classify_result(sender_stats: Dict[str, str], receiver_stats: Dict[str, str]) -> str:
    sender_noack = stat_int(sender_stats, "tx_noack")
    sender_err = stat_int(sender_stats, "tx_err")
    receiver_rx_ok = stat_int(receiver_stats, "rx_ok")

    if sender_noack == 0 and sender_err == 0:
        return "pass"
    if sender_noack > 0 and receiver_rx_ok > 0:
        return "ack-path"
    if sender_noack > 0 and receiver_rx_ok == 0:
        return "payload-loss"
    return "other"


def run_one_way(
    sender: Node,
    receiver: Node,
    case: TestCase,
    packet_count: int,
    interval_ms: int,
) -> DirectionResult:
    sender.run("radio-test reset")
    receiver.run("radio-test reset")
    sender.run(f"radio-test target {receiver.local_mac}")

    sender.run(f"radio-test run {packet_count}")

    # Avoid polling the sender while it is transmitting: that shell traffic
    # perturbs packet timing and can create artificial bursts. Sleep until the
    # run should be nearly done, then verify idle state.
    expected_run_s = (packet_count * interval_ms) / 1000.0
    time.sleep(expected_run_s + 0.5)

    sender_stats = wait_until_idle(sender, timeout_s=10.0)
    receiver_stats = wait_for_receiver_settle(receiver)

    return DirectionResult(
        case=case.name,
        sender=sender.label,
        receiver=receiver.label,
        target_packets=packet_count,
        sender_stats=sender_stats,
        receiver_stats=receiver_stats,
        classification=classify_result(sender_stats, receiver_stats),
    )


def build_cases(args: argparse.Namespace) -> List[TestCase]:
    cases: List[TestCase] = []
    for power in parse_csv_ints(args.powers):
        for channel in parse_csv_ints(args.channels):
            for length in parse_csv_ints(args.lengths):
                for txmax in parse_csv_ints(args.txmax_values):
                    cases.append(TestCase(power=power, channel=channel, length=length, txmax=txmax))
    return cases


def print_case_result(result: DirectionResult) -> None:
    sender = result.sender_stats
    receiver = result.receiver_stats
    print(
        f"{result.case} {result.sender}->{result.receiver}: "
        f"started={stat_int(sender, 'tx_started')} "
        f"ok={stat_int(sender, 'tx_ok')} "
        f"noack={stat_int(sender, 'tx_noack')} "
        f"err={stat_int(sender, 'tx_err')} "
        f"rx_ok={stat_int(receiver, 'rx_ok')} "
        f"class={result.classification}"
    )


def close_nodes(nodes: Iterable[Node]) -> None:
    for node in nodes:
        try:
            node.shell.close()
        except OSError:
            pass


def parse_args(argv: List[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--ports",
        nargs=2,
        metavar=("PORT_A", "PORT_B"),
        help="Use these two serial ports instead of auto-discovery.",
    )
    parser.add_argument(
        "--port-glob",
        action="append",
        default=[],
        help="Additional glob pattern for auto-discovery.",
    )
    parser.add_argument("--count", type=int, default=20, help="Target packets per one-way run.")
    parser.add_argument("--interval-ms", type=int, default=1000, help="Packet interval in milliseconds.")
    parser.add_argument("--powers", default="8", help="Comma-separated TX power list, e.g. 0,8")
    parser.add_argument("--channels", default="26", help="Comma-separated channel list, e.g. 26,20")
    parser.add_argument("--lengths", default="50", help="Comma-separated payload length list.")
    parser.add_argument("--txmax-values", default="1,3", help="Comma-separated max-MAC-transmission list.")
    parser.add_argument("--verbose", type=int, choices=(0, 1), default=0, help="Set radio-test verbose mode.")
    parser.add_argument("--json-out", help="Write full results as JSON to this file.")
    parser.add_argument("--log-dir", help="Write one raw transcript log per serial port to this directory.")
    return parser.parse_args(argv)


def main(argv: List[str]) -> int:
    args = parse_args(argv)
    nodes: List[Node] = []
    log_dir = Path(args.log_dir) if args.log_dir else None

    try:
        if args.ports:
            nodes = [
                connect_node(
                    args.ports[0],
                    "node-a",
                    (log_dir / f"{Path(args.ports[0]).name}.log") if log_dir else None,
                ),
                connect_node(
                    args.ports[1],
                    "node-b",
                    (log_dir / f"{Path(args.ports[1]).name}.log") if log_dir else None,
                ),
            ]
        else:
            patterns = list(DEFAULT_PORT_GLOBS) + args.port_glob
            nodes = autodiscover_nodes(patterns, log_dir=log_dir)

        if nodes[0].local_mac == nodes[1].local_mac:
            raise RuntimeError("Both serial ports reported the same MAC address.")

        print("Discovered nodes:")
        for node in nodes:
            print(f"  {node.label}: port={node.port} mac={node.local_mac}")

        results: List[DirectionResult] = []
        cases = build_cases(args)

        for case in cases:
            print(f"\nCase {case.name}")
            for node in nodes:
                configure_node(node, case, args.interval_ms, args.verbose)

            result_ab = run_one_way(nodes[0], nodes[1], case, args.count, args.interval_ms)
            print_case_result(result_ab)
            results.append(result_ab)

            result_ba = run_one_way(nodes[1], nodes[0], case, args.count, args.interval_ms)
            print_case_result(result_ba)
            results.append(result_ba)

        if args.json_out:
            output_path = Path(args.json_out)
            output_path.parent.mkdir(parents=True, exist_ok=True)
            payload = {
                "generated_at": time.strftime("%Y-%m-%dT%H:%M:%S%z"),
                "nodes": [{"label": node.label, "port": node.port, "local_mac": node.local_mac} for node in nodes],
                "packet_count": args.count,
                "interval_ms": args.interval_ms,
                "results": [asdict(result) for result in results],
            }
            output_path.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")
            print(f"\nWrote JSON results to {output_path}")

        return 0

    except KeyboardInterrupt:
        print("\nInterrupted.", file=sys.stderr)
        return 130
    except Exception as exc:
        print(f"Error: {exc}", file=sys.stderr)
        return 1
    finally:
        close_nodes(nodes)


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
