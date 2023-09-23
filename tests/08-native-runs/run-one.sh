#!/bin/bash

source ../utils.sh

BASENAME=$(basename $1)

cd ${1}
test_init

echo "-- Starting test $1"

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
