#!/bin/bash

# Contiki directory
CONTIKI=$1

# Simulation file
BASENAME=07-native-border-router-sky

bash test-nbr.sh $CONTIKI $BASENAME fd02::0212:7202:0002:0202
