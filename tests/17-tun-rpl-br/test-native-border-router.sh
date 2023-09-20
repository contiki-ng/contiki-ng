#!/bin/bash

# Contiki directory
CONTIKI=$1

# Simulation file
BASENAME=$2

# Destination IPv6
IPADDR=$3

# Time allocated for convergence
WAIT_TIME=$4

# Payload len. Default is ping6's default, 56.
PING_SIZE=${5:-56}

# Inter-ping delay. Default is ping6's default, 1s.
PING_DELAY=${6:-1}

# ICMP request-reply count
COUNT=5

# Connect to the simulation
echo "Starting native border-router"
make -C $CONTIKI/examples/rpl-border-router -B connect-router-cooja TARGET=native &
MPID=$!
printf "Waiting for network formation (%d seconds)\n" "$WAIT_TIME"
sleep $WAIT_TIME

# Do ping
ping6 $IPADDR -c $COUNT -s $PING_SIZE -i $PING_DELAY
STATUS=$?

echo "Closing nbr"
kill $MPID

if [ $STATUS -eq 0 ]; then
  exit 0
else
  exit 1
fi
