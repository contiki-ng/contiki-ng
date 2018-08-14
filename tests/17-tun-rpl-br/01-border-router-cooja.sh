#!/bin/bash

# Contiki directory
CONTIKI=$1

# Simulation file
BASENAME=$(basename $0 .sh)

bash test-border-router.sh $CONTIKI $BASENAME fd00::204:4:4:4 60
