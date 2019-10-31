#!/bin/bash
source ../utils.sh

# Contiki directory
CONTIKI=$1

# Basic statistics for packet parsing results.
SUCCEEDED=0
FAILED=0
TIMEDOUT=0

# Example code directory
CODE_DIR=packet-injector
CODE=packet-injector
PACKET_DIR=$CODE_DIR/$TEST_PROTOCOL-data
echo packet dir = $PACKET_DIR

# Starting Contiki-NG native node
echo "Starting native node"
make -C $CODE_DIR TARGET=native > make.log 2> make.err

for i in $PACKET_DIR/*
do
  export TEST_FILE=$i
  echo Injecting file $TEST_FILE
  timeout -k 1s 2s sh -c "$CODE_DIR/$CODE.native >> $CODE.log 2>> $CODE.err"
  INJECTOR_EXIT_CODE=$?
  echo "exit code:" $INJECTOR_EXIT_CODE

  case $INJECTOR_EXIT_CODE in
  0) echo "Packet $i: SUCCESS"
     SUCCEEDED=$((SUCCEEDED + 1))
     ;;
  124) echo "Packet $i: TIMEOUT"
     TIMEDOUT=$((TIMEDOUT + 1))
     ;;
  *) echo "Packet $i: FAILURE"
     FAILED=$((FAILED + 1))
     ;;
  esac
done

echo "Closing native node"

if [ $((TIMEDOUT + FAILED)) -gt 0 ]; then
  echo "==== make.log ====" ; cat make.log;
  echo "==== make.err ====" ; cat make.err;
  echo "==== $CODE.log ====" ; cat $CODE.log;
  echo "==== $CODE.err ====" ; cat $CODE.err;

  printf "%-32s TEST FAIL\n" "$CODE-$TEST_PROTOCOL" | tee $CODE.testlog;
else
  cp $CODE.log $CODE.testlog
  printf "%-32s TEST OK\n" "$CODE-$TEST_PROTOCOL" | tee $CODE.testlog;
fi

rm make.log
rm make.err
rm $CODE.log
rm $CODE.err

echo "Succeeded: " $SUCCEEDED
echo "Timed out: " $TIMEDOUT
echo "Failed   : " $FAILED

sleep 3

# We do not want Make to stop -> Return 0
# The Makefile will check if a log contains FAIL at the end
exit 0
