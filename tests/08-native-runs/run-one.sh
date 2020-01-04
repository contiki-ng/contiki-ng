#!/bin/bash

source ../utils.sh

BASENAME=$(basename $1)
BUILDLOG=$BASENAME.build.log

cd ${1}
test_init

register_logfile $BUILDLOG

echo "-- Starting test $1"
# Clean and build
assert "clean" "make clean &> $BUILDLOG"
assert "compile" "make -j >> $BUILDLOG 2>&1"

for TEST in ./test*.native; do
  RUNLOG=$(basename $TEST .native).run.log
  register_logfile $RUNLOG

  # Start test in background
  $TEST &> $RUNLOG &
  register_last_bg_cmd

  wait_log_assert "start $TEST" "Run unit-test" $RUNLOG 30
  wait_log_assert "run $TEST" "=check-me= DONE" $RUNLOG 120
  assert "check $TEST" "! grep -q '=check-me= FAILED' $RUNLOG"
done

do_wrap_up
