#!/bin/bash

# Contiki directory
CONTIKI=$1
# Test basename
BASENAME=01-native-ping

IPADDR=fd00::302:304:506:708

# Starting Contiki-NG native node
echo "Starting native node"
make -C $CONTIKI/examples/hello-world
sudo $CONTIKI/examples/hello-world/hello-world.native > node.log 2> node.err &
CPID=$!
sleep 2

# Do ping
echo "Pinging"
ping6 $IPADDR -c 5 | tee $BASENAME.log
# Fetch ping6 status code (not $? because this is piped)
STATUS=${PIPESTATUS[0]}

echo "Closing native node"
sleep 2
sudo kill -9 $CPID

if [ $STATUS -eq 0 ] ; then
  mv $BASENAME.log $BASENAME.testlog
  echo " OK"
else
  mv $BASENAME.log $BASENAME.faillog

  echo ""
  echo "---- node.log"
  cat node.log

  echo ""
  echo "---- node.err"
  cat node.err

  echo " FAIL ಠ_ಠ" | tee -a $BASENAME.faillog;
fi

# We do not want Make to stop -> Return 0
# The Makefile will check if a log contains FAIL at the end

exit 0
