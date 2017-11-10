#!/bin/bash

# Contiki directory
CONTIKI=$1

# Simulation file
BASENAME=$2

# Destination IPv6
IPADDR=$3

# Start simulation
echo "Starting Cooja simulation $BASENAME.csc"
java -Xshare:on -jar $CONTIKI/tools/cooja/dist/cooja.jar -nogui=$BASENAME.csc -contiki=$CONTIKI > /dev/null &
JPID=$!
sleep 20

# Connect to the simlation
echo "Starting tunslip6"
make -C $CONTIKI/tools tunslip6
make -C $CONTIKI/examples/rpl-border-router/ connect-router-cooja TARGET=zoul > tunslip.log 2> tunslip.err &
MPID=$!
echo "Waiting for network formation"
sleep 5

# Do ping
echo "Pinging"
ping6 $IPADDR -c 5 | tee $BASENAME.log
# Fetch ping6 status code (not $? because this is piped)
STATUS=${PIPESTATUS[0]}

echo "Closing simulation and tunslip6"
sleep 2
kill -9 $JPID
kill -9 $MPID

if [ $STATUS -eq 0 ] ; then
  mv $BASENAME.log $BASENAME.testlog
  echo " OK"
else
  mv $BASENAME.log $BASENAME.faillog

  echo ""
  echo "---- COOJA.log"
  cat COOJA.log

  echo ""
  echo "---- tunslip.log"
  cat tunslip.log

  echo ""
  echo "---- tunslip.err"
  cat tunslip.err

  echo " FAIL ಠ_ಠ" | tee -a $BASENAME.faillog;
fi

# We do not want Make to stop -> Return 0
# The Makefile will check if a log contains FAIL at the end

exit 0
