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

# Start simulation
echo "Starting Cooja simulation $BASENAME.csc"
java -Xshare:on -jar $CONTIKI/tools/cooja/dist/cooja.jar -nogui=$BASENAME.csc -contiki=$CONTIKI > $BASENAME.coojalog &
JPID=$!
sleep 20

# Connect to the simulation
echo "Starting tunslip6"
make -C $CONTIKI/examples/rpl-border-router/ connect-router-cooja TARGET=zoul >> $BASENAME.tunslip6.log 2>&1 &
MPID=$!
printf "Waiting for network formation (%d seconds)\n" "$WAIT_TIME"
sleep $WAIT_TIME

# Do ping
echo "Pinging"
ping6 $IPADDR -c $COUNT -s $PING_SIZE -i $PING_DELAY | tee $BASENAME.scriptlog
# Fetch ping6 status code (not $? because this is piped)
STATUS=${PIPESTATUS[0]}
REPLIES=`grep -c 'icmp_seq=' $BASENAME.scriptlog`

echo "Closing simulation and tunslip6"
sleep 1
kill_bg $JPID
kill_bg $MPID
sleep 1
rm COOJA.testlog
rm COOJA.log

if [ $STATUS -eq 0 ] && [ $REPLIES -eq $COUNT ] ; then
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
