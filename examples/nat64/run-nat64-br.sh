#!/bin/bash
#
# Start the native border router with NAT64 enabled.
#
# Usage:
#   sudo ./run-nat64-br.sh                  # connect to Cooja on localhost:60001
#   sudo ./run-nat64-br.sh /dev/ttyUSB0     # connect to a hardware radio dongle

CONTIKI_DIR=$(cd "$(dirname "$0")/../.." && pwd)
BR_DIR="$CONTIKI_DIR/examples/rpl-border-router"

echo "Building native border router..."
make -C "$BR_DIR" TARGET=native -j"$(nproc)" || exit 1

if [ -n "$1" ]; then
    echo "Starting border router with NAT64 (serial device $1)..."
    "$BR_DIR/build/native/border-router.native" \
        -s "$1" --nat64 fd00::1/64
else
    echo "Starting border router with NAT64 (connecting to Cooja on port 60001)..."
    "$BR_DIR/build/native/border-router.native" \
        -a localhost -p 60001 --nat64 fd00::1/64
fi
