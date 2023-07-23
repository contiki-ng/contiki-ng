#!/bin/sh -e

VALGRIND_CMD="valgrind" MQTT_VERSION="3_1_1" ./mqtt-client.sh "$@"
