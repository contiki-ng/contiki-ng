#!/bin/bash

# Contiki directory
CONTIKI=$1

# Simulation file
BASENAME=07-native-border-router-cooja

bash test-nbr.sh $CONTIKI $BASENAME fd00::204:4:4:4
