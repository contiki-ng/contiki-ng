#!/bin/bash
source ../utils.sh

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

CURDIR=$(pwd)

# Start simulation
ant -e -logger org.apache.tools.ant.listener.SimpleBigProjectLogger -f $CONTIKI/tools/cooja/build.xml run_bigmem -Dargs="-nogui=$CURDIR/$BASENAME.csc -contiki=$CONTIKI -logdir=$CURDIR" &
JPID=$!
sleep 30

# Connect to the simulation
echo "Starting tunslip6"
make -C $CONTIKI/examples/rpl-border-router connect-router-cooja TARGET=zoul &
MPID=$!
printf "Waiting for network formation (%d seconds)\n" "$WAIT_TIME"
sleep $WAIT_TIME

# Do ping
ping6 $IPADDR -c $COUNT -s $PING_SIZE -i $PING_DELAY
STATUS=$?

echo "Closing simulation and tunslip6"
sleep 1
kill_bg $JPID 15
kill_bg $MPID 15
sleep 1
rm -f COOJA.testlog COOJA.log

if [ $STATUS -eq 0 ]; then
  printf "%-32s TEST OK\n" "$BASENAME" | tee $BASENAME.testlog;
else
  printf "%-32s TEST FAIL\n" "$BASENAME" | tee $BASENAME.testlog;
fi

# We do not want Make to stop -> Return 0
# The Makefile will check if a log contains FAIL at the end

exit 0
