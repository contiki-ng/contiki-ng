#!/bin/bash
source ../utils.sh

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
for i in $PACKET_DIR/*
do
  if [ -d "$i" ]; then
    test_files=()
    for f in $i/*
    do
      test_files+=("$f")
    done
    echo Injecting ${#test_files[@]} files in $i
  else
    test_files=("$i")
    echo Injecting file $i
  fi
  timeout -k 1s 2s "$CODE_DIR/$CODE.native" "${test_files[@]}"
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
  printf "%-32s TEST FAIL\n" "$CODE-$TEST_PROTOCOL"

  echo "Succeeded: " $SUCCEEDED
  echo "Timed out: " $TIMEDOUT
  echo "Failed   : " $FAILED
  exit 1
fi
