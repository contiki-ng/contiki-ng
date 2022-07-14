#!/bin/bash
source ../utils.sh

# Contiki directory
CONTIKI=$1

# Simulation file
BASENAME=$(basename $0 .sh)

# Destination IPv6
IPADDR=fd00::204:4:4:4

# Time allocated for toplogy formation
WAIT_TIME=60

# The expected hop count
TARGETHOPS=4

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
echo "Running Traceroute"
traceroute6 $IPADDR -m 5 | tee $BASENAME.scriptlog
# Fetch traceroute6 status code (not $? because this is piped)
STATUS=${PIPESTATUS[0]}
HOPS=`wc $BASENAME.scriptlog -l | cut -f 1 -d ' '`

echo "Closing simulation and tunslip6"
sleep 1
kill_bg $JPID 15
kill_bg $MPID 15
sleep 1
rm -f COOJA.testlog COOJA.log

if [ $STATUS -eq 0 ] && [ $HOPS -eq $TARGETHOPS ] ; then
  printf "%-32s TEST OK\n" "$BASENAME" | tee $BASENAME.testlog;
else
  printf "%-32s TEST FAIL\n" "$BASENAME" | tee $BASENAME.testlog;
fi

# We do not want Make to stop -> Return 0
# The Makefile will check if a log contains FAIL at the end

exit 0
