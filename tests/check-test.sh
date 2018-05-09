#!/bin/bash
FAILS=`grep -c  'TEST FAIL' $1/summary`
echo "======== Test outcome ======="
if [ -f "$1/summary" ] && [ $FAILS == 0 ]; then
  printf "Test %s OK\n" "$1"
  exit 0
else
  printf "Test %s FAIL\n" "$1"
  exit 1
fi
