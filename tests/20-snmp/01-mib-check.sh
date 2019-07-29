#!/bin/bash
source ../utils.sh

# Contiki directory
CONTIKI=$1
# Test basename
BASENAME=01-mib-check
# Report file
REPORT=report.txt

smilint -s -e -l 3 \
    $CONTIKI/os/net/app-layer/snmp/CONTIKI-NG.mib \
    $CONTIKI/os/net/app-layer/snmp/CONTIKI-NG-ENERGEST.mib \
    2>$REPORT

LINES=$(cat $REPORT | wc -l)

if [ $LINES -eq 0 ]; then
    echo "${BASENAME} TEST OK" > ${BASENAME}.testlog
else
    echo "${BASENAME} TEST FAIL" > ${BASENAME}.testlog
fi

cat $REPORT

rm $REPORT

# We do not want Make to stop -> Return 0
# The Makefile will check if a log contains FAIL at the end
exit 0
