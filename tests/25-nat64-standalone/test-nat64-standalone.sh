#!/bin/bash
#
# Round-trip test for the standalone native NAT64 translator
# (os/services/nat64/native/standalone). Needs no Cooja, border router, tun,
# sudo, or internet:
#   1. Build a plain native uIP UDP probe with the standalone module. The
#      module makes the node translate its own NAT64 traffic over host sockets,
#      enables NAT64 at compile time, and suppresses the default route.
#   2. Start a Python IPv4 echo server on 127.0.0.1 (reused from
#      tests/17-tun-rpl-br).
#   3. Run the probe. It sends "PING-NAT64" to 64:ff9b::7f00:1 (= 127.0.0.1 via
#      the well-known NAT64 prefix); the translator turns it into an IPv4
#      loopback datagram, the server reflects it, and the translator turns the
#      reply back into IPv6.
#   4. Assert the server saw the IPv4 datagram (IPv6->IPv4 leg) and the probe
#      logged NAT64_ECHO_OK (IPv4->IPv6 return leg).
#
# The probe build sets NAT64_CONF_ALLOW_LOOPBACK=1 (project-conf.h) so the
# whole exchange stays on the loopback interface.
#
# Author: Nicolas Tsiftes <nicolas.tsiftes@ri.se>

set -u

THIS_DIR="$(cd "$(dirname "$0")" && pwd)"
CONTIKI="$THIS_DIR/../.."
PROBE_DIR="$THIS_DIR/code-nat64-standalone"
ECHO_SERVER="$CONTIKI/tests/17-tun-rpl-br/nat64-echo-server.py"

UDP_PORT=5557
TCP_PORT=5558
RUN_TIME=20

PROBE_BIN="$PROBE_DIR/build/native/nat64-standalone-test.native"
BUILD_LOG="$THIS_DIR/nat64-standalone.buildlog"
PROBE_LOG="$THIS_DIR/nat64-standalone.probelog"
ECHO_LOG="$THIS_DIR/nat64-standalone.echo.log"
ECHO_PIDFILE="$THIS_DIR/nat64-standalone.echo.pid"

cleanup() {
  if [ -f "$ECHO_PIDFILE" ]; then
    PID=$(cat "$ECHO_PIDFILE" 2>/dev/null || true)
    [ -n "$PID" ] && kill "$PID" 2>/dev/null || true
    rm -f "$ECHO_PIDFILE"
  fi
}
trap cleanup EXIT INT TERM

rm -f "$BUILD_LOG" "$PROBE_LOG" "$ECHO_LOG" "$ECHO_PIDFILE"

echo "Building standalone NAT64 probe"
if ! make -C "$PROBE_DIR" -B TARGET=native nat64-standalone-test \
     >"$BUILD_LOG" 2>&1; then
  echo "FAIL: probe build failed"
  tail -n 40 "$BUILD_LOG"
  exit 1
fi

echo "Starting IPv4 echo server (UDP $UDP_PORT) on 127.0.0.1"
python3 "$ECHO_SERVER" \
  --host 127.0.0.1 \
  --udp-port "$UDP_PORT" \
  --tcp-port "$TCP_PORT" \
  --log "$ECHO_LOG" \
  --pidfile "$ECHO_PIDFILE" &

# Wait for the server to bind before launching the probe.
for _ in 1 2 3 4 5; do
  grep -q "UDP_LISTEN" "$ECHO_LOG" 2>/dev/null && break
  sleep 1
done
if ! grep -q "UDP_LISTEN" "$ECHO_LOG" 2>/dev/null; then
  echo "FAIL: echo server did not start"
  cat "$ECHO_LOG" 2>/dev/null || true
  exit 1
fi

echo "Running probe (up to ${RUN_TIME}s)"
# The probe exits on its own once it gets the echo back (NAT64_TEST_DONE); the
# timeout is only a backstop.
timeout "$RUN_TIME" "$PROBE_BIN" >"$PROBE_LOG" 2>&1

STATUS=0

if grep -q "UDP_ECHO .*payload=b'PING-NAT64'" "$ECHO_LOG" 2>/dev/null; then
  echo "PASS: echo server received the translated IPv4 datagram"
else
  echo "FAIL: echo server saw no PING-NAT64 datagram (IPv6->IPv4 leg)"
  STATUS=1
fi

if grep -q "NAT64_ECHO_OK" "$PROBE_LOG" 2>/dev/null; then
  echo "PASS: probe received the reflected datagram back over IPv6"
else
  echo "FAIL: probe did not log NAT64_ECHO_OK (IPv4->IPv6 return leg)"
  STATUS=1
fi

if [ $STATUS -ne 0 ]; then
  echo "==== probe log ===="
  cat "$PROBE_LOG" 2>/dev/null || true
  echo "==== echo server log ===="
  cat "$ECHO_LOG" 2>/dev/null || true
fi

exit $STATUS
