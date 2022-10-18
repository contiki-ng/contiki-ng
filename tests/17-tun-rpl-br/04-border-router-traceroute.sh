#!/bin/bash

# Contiki directory
CONTIKI=$1

# Simulation file
BASENAME=$2

# Destination IPv6
IPADDR=fd00::204:4:4:4

# Time allocated for toplogy formation
WAIT_TIME=60

# The expected hop count
TARGETHOPS=4

# Connect to the simulation
echo "Starting tunslip6"
make -C $CONTIKI/examples/rpl-border-router connect-router-cooja TARGET=zoul &
MPID=$!
printf "Waiting for network formation (%d seconds)\n" "$WAIT_TIME"
sleep $WAIT_TIME

echo "Running mtr (traceroute)"
# IPv6, 5 hops max, no DNS lookups, 3 pings each, output with report mode.
mtr -6 -m 5 -n -c 3 -r $IPADDR | tee $BASENAME.scriptlog
# Fetch mtr status code (not $? because this is piped)
STATUS=${PIPESTATUS[0]}
HOPS=`grep -e "^ " $BASENAME.scriptlog | wc -l`
rm -f $BASENAME.scriptlog

echo "Closing tunslip6"
kill $MPID

if [ $STATUS -eq 0 ] && [ $HOPS -eq $TARGETHOPS ] ; then
  printf "%-32s TEST OK\n" "$BASENAME" > $BASENAME.testlog
  exit 0
else
  printf "%-32s TEST FAIL\n" "$BASENAME" > $BASENAME.testlog
  exit 1
fi
