#!/bin/sh -e

MQTT_VERSION="3_1" VALGRIND_CMD="valgrind" ./mqtt-client.sh "$@"
