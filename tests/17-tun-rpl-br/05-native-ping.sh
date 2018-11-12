#!/bin/bash
source ../utils.sh

# Contiki directory
CONTIKI=$1
# Test basename
BASENAME=$(basename $0 .sh)

IPADDR=fd00::302:304:506:708

# Starting Contiki-NG native node
echo "Starting native node"
make -C $CONTIKI/examples/hello-world > make.log 2> make.err
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
kill_bg $CPID

if [ $STATUS -eq 0 ] ; then
  cp $BASENAME.log $BASENAME.testlog
  printf "%-32s TEST OK\n" "$BASENAME" | tee $BASENAME.testlog;
else
  echo "==== make.log ====" ; cat make.log;
  echo "==== make.err ====" ; cat make.err;
  echo "==== node.log ====" ; cat node.log;
  echo "==== node.err ====" ; cat node.err;
  echo "==== $BASENAME.log ====" ; cat $BASENAME.log;

  printf "%-32s TEST FAIL\n" "$BASENAME" | tee $BASENAME.testlog;
fi

rm make.log
rm make.err
rm node.log
rm node.err

# We do not want Make to stop -> Return 0
# The Makefile will check if a log contains FAIL at the end
exit 0
