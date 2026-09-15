#!/bin/bash

source ../utils.sh

BIN_PREFIX=${TEST_PREFIX:-test}
BASENAME=$(basename $1)

cd ${1}
test_init

# The logs of a previous run are named after the binary rather than after
# the test, so test_init does not remove them. Left in place they are what
# the assertions below read when a run produces nothing of its own.
rm -f ./*.run.log

echo "-- Starting test $1"

# Without this an unmatched pattern is passed on as a literal name, and
# everything below then works on that instead of on a binary.
shopt -s nullglob
TESTS=(./build/native/${BIN_PREFIX}*.native)
shopt -u nullglob

if [ ${#TESTS[@]} -eq 0 ]; then
  echo "No ${BIN_PREFIX}*.native in $1/build/native; was it built?"
  TEST_OK=0
  do_wrap_up
fi

for TEST in "${TESTS[@]}"; do
  RUNLOG=$(basename "$TEST" .native).run.log
  register_logfile "$RUNLOG"

  # Start test in background
  "$TEST" &> "$RUNLOG" &
  register_last_bg_cmd

  wait_log_assert "start $TEST" "Run unit-test" "$RUNLOG" 30
  wait_log_assert "run $TEST" "=check-me= DONE" "$RUNLOG" 120
  assert "check $TEST" "! grep -q '=check-me= FAILED' $RUNLOG"
done

do_wrap_up
