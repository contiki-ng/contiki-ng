#!/bin/sh -e

VALGRIND_CMD="valgrind" MQTT_VERSION="5" ./mqtt-client.sh "$@"
