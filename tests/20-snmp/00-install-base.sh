#!/bin/bash
source ../utils.sh

# Contiki directory
CONTIKI=$1
# Test basename
BASENAME=00-install-base

sudo apt-get -qq update && sudo apt-get -qq install -y smitools snmp snmp-mibs-downloader

if [ $? -eq 0 ]; then
    echo "${BASENAME} TEST OK" > ${BASENAME}.testlog
else
    echo "${BASENAME} TEST FAIL" > ${BASENAME}.testlog
fi

# We do not want Make to stop -> Return 0
# The Makefile will check if a log contains FAIL at the end
exit 0
