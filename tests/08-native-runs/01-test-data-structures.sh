#!/bin/bash
source ../utils.sh

# Contiki directory
CONTIKI=../..

# Example code directory
CODE_DIR=$CONTIKI/tests/07-simulation-base/code-data-structures/
CODE=test-data-structures

# Starting Contiki-NG native node
echo "Starting native node"
$CODE_DIR/build/native/$CODE.native > $CODE.log 2> $CODE.err &
CPID=$!

echo "Closing native node"
sleep 2
kill_bg $CPID

# A run that never reached its end reports neither, so both are checked:
# without the second test, a node that failed to build or to start at all
# leaves no marker and the test passes on an empty log.
if grep -q "=check-me= FAILED" $CODE.log \
   || ! grep -q "=check-me= DONE" $CODE.log ; then
  echo "==== $CODE.log ====" ; cat $CODE.log;
  echo "==== $CODE.err ====" ; cat $CODE.err;
  printf "%-32s TEST FAIL\n" "$CODE" | tee $CODE.testlog;
  rm -f $CODE.log $CODE.err
  exit 1
fi

rm -f $CODE.log $CODE.err
