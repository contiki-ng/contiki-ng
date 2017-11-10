#!/bin/bash

# Contiki directory
CONTIKI=$1

# Simulation file
BASENAME=02-border-router-cooja-tsch

bash test-border-router.sh $CONTIKI $BASENAME fd00::204:4:4:4
