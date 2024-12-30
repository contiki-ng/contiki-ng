#!/bin/bash
source ../utils.sh

# Contiki directory
CONTIKI=../..
# Test basename
BASENAME=$(basename $0 .sh)

IPADDR=fd00::302:304:506:708

# Starting Contiki-NG native node
echo "Starting native node"
sudo $CONTIKI/examples/hello-world/build/native/hello-world.native &
CPID=$!
sleep 2

# Do ping
echo "Pinging"
ping6 $IPADDR -c 5 | tee $BASENAME.log
# Fetch ping6 status code (not $? because this is piped)
STATUS=${PIPESTATUS[0]}

echo "Closing native node"
sleep 2
kill_bg $CPID

if [ $STATUS -eq 0 ] ; then
  cp $BASENAME.log $BASENAME.testlog
  printf "%-32s TEST OK\n" "$BASENAME" | tee $BASENAME.testlog;
else
  echo "==== $BASENAME.log ====" ; cat $BASENAME.log;

  printf "%-32s TEST FAIL\n" "$BASENAME" | tee $BASENAME.testlog;
  exit 1
fi
