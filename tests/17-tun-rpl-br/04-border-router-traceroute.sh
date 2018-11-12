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

# Start simulation
echo "Starting Cooja simulation $BASENAME.csc"
java -Xshare:on -jar $CONTIKI/tools/cooja/dist/cooja.jar -nogui=$BASENAME.csc -contiki=$CONTIKI  > $BASENAME.coojalog &
JPID=$!
sleep 20

# Connect to the simulation
echo "Starting tunslip6"
make -C $CONTIKI/examples/rpl-border-router/ connect-router-cooja TARGET=zoul >> $BASENAME.tunslip6.log 2>&1 &
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
kill_bg $JPID
kill_bg $MPID
sleep 1
rm COOJA.testlog
rm COOJA.log

if [ $STATUS -eq 0 ] && [ $HOPS -eq $TARGETHOPS ] ; then
  printf "%-32s TEST OK\n" "$BASENAME" | tee $BASENAME.testlog;
else
  echo "==== $BASENAME.coojalog ====" ; cat $BASENAME.coojalog;
  echo "==== $BASENAME.tunslip6.log ====" ; cat $BASENAME.tunslip6.log;
  echo "==== $BASENAME.scriptlog ====" ; cat $BASENAME.scriptlog;

  printf "%-32s TEST FAIL\n" "$BASENAME" | tee $BASENAME.testlog;
fi

# We do not want Make to stop -> Return 0
# The Makefile will check if a log contains FAIL at the end

exit 0
