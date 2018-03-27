#!/bin/bash

# Contiki directory
CONTIKI=$1

# Simulation file
BASENAME=01-border-router-cooja

bash test-border-router.sh $CONTIKI $BASENAME fd00::204:4:4:4 60
