#!/bin/bash

# Contiki directory
CONTIKI=$1

# Simulation file
BASENAME=03-border-router-sky

bash test-border-router.sh $CONTIKI $BASENAME fd00::0212:7404:0004:0404 60
