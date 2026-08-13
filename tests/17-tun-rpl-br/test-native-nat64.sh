#!/bin/bash
#
# End-to-end test for the NAT64 gateway over a multi-hop RPL network.
#
# Driven by Cooja's SerialSocketServer plugin: the .csc starts a
# slip-radio mote and several nat64-test motes positioned in a chain,
# then runs this script.  This script:
#   1. Starts a Python IPv4 echo server on 127.0.0.1 (UDP and TCP).
#   2. Starts the native rpl-border-router with --nat64 enabled,
#      connecting to Cooja's serial socket on localhost:60001.
#   3. Waits long enough for RPL convergence over multiple hops and
#      for every mote to round-trip both UDP and TCP probes through
#      the gateway.
#   4. For each expected node_id, verifies that
#        - the echo server saw a "PING-<id>" payload on UDP and TCP
#          (proves the IPv6->IPv4 leg works for that mote), and
#        - the mote logged "UDP_ECHO_OK node=<id>" and
#          "TCP_ECHO_OK node=<id>" (proves the IPv4->IPv6 return leg).
#
# Usage: test-native-nat64.sh CONTIKI_DIR BASENAME WAIT_TIME [NODE_IDS...]
#

set -u

CONTIKI=$1
BASENAME=$2
WAIT_TIME=${3:-180}
shift 3 || true
NODE_IDS=("$@")
if [ ${#NODE_IDS[@]} -eq 0 ]; then
  NODE_IDS=(2 3 4)
fi

THIS_DIR="$(cd "$(dirname "$0")" && pwd)"
ECHO_LOG="$THIS_DIR/$BASENAME.echo.log"
ECHO_PIDFILE="$THIS_DIR/$BASENAME.echo.pid"
BR_LOG="$THIS_DIR/$BASENAME.brlog"
COOJA_LOG="$THIS_DIR/COOJA.testlog"

UDP_PORT=5557
TCP_PORT=5558

cleanup() {
  if [ -f "$ECHO_PIDFILE" ]; then
    PID=$(cat "$ECHO_PIDFILE" 2>/dev/null || true)
    if [ -n "$PID" ]; then
      kill "$PID" 2>/dev/null || true
    fi
    rm -f "$ECHO_PIDFILE"
  fi
  if [ -n "${MPID:-}" ]; then
    kill "$MPID" 2>/dev/null || true
  fi
}
trap cleanup EXIT INT TERM

rm -f "$ECHO_LOG" "$ECHO_PIDFILE" "$BR_LOG"

echo "Starting IPv4 echo server (UDP $UDP_PORT, TCP $TCP_PORT)"
python3 "$THIS_DIR/nat64-echo-server.py" \
  --host 127.0.0.1 \
  --udp-port "$UDP_PORT" \
  --tcp-port "$TCP_PORT" \
  --log "$ECHO_LOG" \
  --pidfile "$ECHO_PIDFILE" &

# Give the server a moment to bind both sockets.
for _ in 1 2 3 4 5; do
  if grep -q "TCP_LISTEN" "$ECHO_LOG" 2>/dev/null && \
     grep -q "UDP_LISTEN" "$ECHO_LOG" 2>/dev/null; then
    break
  fi
  sleep 1
done

echo "Starting native border-router with NAT64"
# NAT64_ALLOW_LOOPBACK=1 lets the gateway forward to 127.0.0.0/8 so it
# can reach the echo server on the BR's loopback.  Test-only — the BR
# rejects loopback destinations by default.
#
# --no-tun lets the BR run without CAP_NET_ADMIN/sudo: NAT64 traffic is
# served entirely from the BR's own AF_INET sockets, and any non-NAT64
# IPv6 traffic from the motes is dropped at the fallback interface.
make -C "$CONTIKI/examples/rpl-border-router" -B \
     border-router.native TARGET=native NAT64_ALLOW_LOOPBACK=1 \
     >"$BR_LOG" 2>&1
"$CONTIKI/examples/rpl-border-router/build/native/border-router.native" \
     -a localhost --nat64 --no-tun fd00::1/64 \
     >>"$BR_LOG" 2>&1 &
MPID=$!

echo "Waiting $WAIT_TIME seconds for convergence and probes (nodes: ${NODE_IDS[*]})"
sleep "$WAIT_TIME"

STATUS=0

# Per-node assertions.  Each mote tags its payload as PING-<id> and logs
# "UDP_ECHO_OK node=<id>" / "TCP_ECHO_OK node=<id>" on a successful echo.
for ID in "${NODE_IDS[@]}"; do
  if grep -q "payload=b'PING-${ID}'" "$ECHO_LOG" 2>/dev/null; then
    : # Match either UDP or TCP — we tighten below.
  fi

  if grep -q "UDP_ECHO .*payload=b'PING-${ID}'" "$ECHO_LOG" 2>/dev/null; then
    echo "PASS: echo server reflected UDP PING-${ID}"
  else
    echo "FAIL: echo server saw no UDP datagram from node ${ID}"
    STATUS=1
  fi

  if grep -q "TCP_ECHO .*payload=b'PING-${ID}'" "$ECHO_LOG" 2>/dev/null; then
    echo "PASS: echo server reflected TCP PING-${ID}"
  else
    echo "FAIL: echo server saw no TCP payload from node ${ID}"
    STATUS=1
  fi

  if [ -f "$COOJA_LOG" ]; then
    if grep -q "UDP_ECHO_OK node=${ID}" "$COOJA_LOG"; then
      echo "PASS: mote ${ID} received the UDP echo back"
    else
      echo "FAIL: mote ${ID} did not log UDP_ECHO_OK"
      STATUS=1
    fi

    if grep -q "TCP_ECHO_OK node=${ID}" "$COOJA_LOG"; then
      echo "PASS: mote ${ID} received the TCP echo back"
    else
      echo "FAIL: mote ${ID} did not log TCP_ECHO_OK"
      STATUS=1
    fi
  else
    echo "FAIL: Cooja log not found at $COOJA_LOG"
    STATUS=1
    break
  fi
done

if [ $STATUS -eq 0 ]; then
  printf "%-32s TEST OK\n" "$BASENAME" >"$BASENAME.testlog"
else
  printf "%-32s TEST FAIL\n" "$BASENAME" >"$BASENAME.testlog"
  echo "==== border-router log (tail) ===="
  tail -n 80 "$BR_LOG" 2>/dev/null || true
  echo "==== echo server log ===="
  cat "$ECHO_LOG" 2>/dev/null || true
fi

exit $STATUS
