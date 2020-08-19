#!/bin/bash
source ../utils.sh

# Contiki directory
CONTIKI=$1

# Example code directory
CODE_DIR=$CONTIKI/tests/07-simulation-base/code-cbor/
CODE=test-cbor

# Starting Contiki-NG native node
echo "Starting native node"
make -C $CODE_DIR TARGET=native > make.log 2> make.err
retVal=$?
if [ $retVal -ne 0 ]; then
    echo "==== make.log ====" ; cat make.log;
    echo "==== make.err ====" ; cat make.err;

    printf "%-32s TEST FAIL\n" "$CODE";
else
  $CODE_DIR/$CODE.native > $CODE.log 2> $CODE.err &
  CPID=$!
  sleep 2

  echo "Closing native node"
  sleep 2
  kill_bg $CPID

  if grep -q "=check-me= FAILED" $CODE.log ; then
    echo "==== make.log ====" ; cat make.log;
    echo "==== make.err ====" ; cat make.err;
    echo "==== $CODE.log ====" ; cat $CODE.log;
    echo "==== $CODE.err ====" ; cat $CODE.err;

    printf "%-32s TEST FAIL\n" "$CODE" | tee $CODE.testlog;
  else
    cp $CODE.log $CODE.testlog
    printf "%-32s TEST OK\n" "$CODE" | tee $CODE.testlog;
  fi

  rm $CODE.log
  rm $CODE.err
fi

rm make.log
rm make.err

# We do not want Make to stop -> Return 0
# The Makefile will check if a log contains FAIL at the end
exit 0
