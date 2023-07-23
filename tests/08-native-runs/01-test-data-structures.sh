#!/bin/bash
source ../utils.sh

# Contiki directory
CONTIKI=$1

# Example code directory
CODE_DIR=$CONTIKI/tests/07-simulation-base/code-data-structures/
CODE=test-data-structures

# Starting Contiki-NG native node
echo "Starting native node"
make -j4 -C $CODE_DIR TARGET=native || exit 1
$CODE_DIR/$CODE.native > $CODE.log 2> $CODE.err &
CPID=$!

echo "Closing native node"
sleep 2
kill_bg $CPID

if grep -q "=check-me= FAILED" $CODE.log ; then
  echo "==== $CODE.log ====" ; cat $CODE.log;
  echo "==== $CODE.err ====" ; cat $CODE.err;
  printf "%-32s TEST FAIL\n" "$CODE" | tee $CODE.testlog;
  rm -f $CODE.log $CODE.err
  exit 1
fi

rm -f $CODE.log $CODE.err
