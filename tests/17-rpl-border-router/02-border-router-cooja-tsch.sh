#!/bin/bash

# Contiki directory
CONTIKI=$1

# Simulation file
BASENAME=02-border-router-cooja-tsch

bash border-router.sh $CONTIKI $BASENAME
