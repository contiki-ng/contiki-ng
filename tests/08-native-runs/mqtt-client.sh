#!/bin/bash
source ../utils.sh

# Contiki directory
CONTIKI=$1

# 3_1, 3_1_1, or 5
# default: 3_1
MQTT_VERSION=${MQTT_VERSION=3_1}

# Example code directory
CODE_DIR=$CONTIKI/examples/mqtt-client/
CODE=mqtt-client

TEST_NAME=$CODE-$MQTT_VERSION
if [ -n "$VALGRIND_CMD" ]
then
  TEST_NAME+="-valgrind"
fi

CLIENT_LOG=$TEST_NAME.log
CLIENT_TESTLOG=$TEST_NAME.testlog
CLIENT_ERR=$TEST_NAME.err
MOSQ_SUB_LOG=mosquitto_sub.log
MOSQ_SUB_ERR=mosquitto_sub.err
PUB_PAYLD=1

# Start mosquitto server
echo "Starting mosquitto daemon"
echo "listener 1883" > mosquitto.conf
echo "allow_anonymous true" >> mosquitto.conf
mosquitto -c mosquitto.conf &> /dev/null &
MOSQID=$!
sleep 2

# Start mosquitto_sub client. Subscribe
echo "Starting mosquitto subscriber"
mosquitto_sub -t iot-2/evt/status/fmt/json > $MOSQ_SUB_LOG 2> $MOSQ_SUB_ERR &
MSUBID=$!
sleep 2

# Starting Contiki-NG native node
echo "Starting native node"
make -C $CODE_DIR -B TARGET=native clean || exit 1
make -j4 -C $CODE_DIR -B TARGET=native \
  DEFINES=MQTT_CLIENT_CONF_ORG_ID=\\\"travis-test\\\",MQTT_CLIENT_CONF_LOG_LEVEL=LOG_LEVEL_DBG,MQTT_CONF_VERSION=MQTT_PROTOCOL_VERSION_$MQTT_VERSION || exit 1
sudo $VALGRIND_CMD $CODE_DIR/$CODE.native > $CLIENT_LOG 2> $CLIENT_ERR &
CPID=$!

# The mqtt-client will publish every 30 secs. Wait for 45
sleep 45

# Send a publish to the mqtt client
echo "Publishing"
mosquitto_pub -m "$PUB_PAYLD" -t iot-2/cmd/leds/fmt/json

echo "Closing native node"
sleep 2
# If we're running Valgrind, we want it to dump the final report, so we don't SIGKILL it
kill_bg $CPID SIGTERM
sleep 1

echo "Stopping mosquitto daemon"
kill_bg $MOSQID

echo "Stopping mosquitto subscriber"
kill_bg $MSUBID

# Success criteria:
# * mosquitto_sub output not empty
# * mqtt-client.native output contains "MQTT SUB"
# * mqtt-client.native payload output matches published payload
# * Valgrind off or 0 errors reported
SUB_RCV=`grep "MQTT SUB" $CLIENT_LOG`
SUB_RCV_PAYLD=`sed -rn "s/.*chunk='(.*)'/\1/p" $CLIENT_LOG`
VALGRIND_NO_ERR=`grep "ERROR SUMMARY: 0 errors from 0 contexts (suppressed: 0 from 0)" $CLIENT_ERR`

if [ -s "$MOSQ_SUB_LOG" -a -n "$SUB_RCV" -a "$SUB_RCV_PAYLD" -eq "$PUB_PAYLD" ] && [ -z "$VALGRIND_CMD" -o -n "$VALGRIND_NO_ERR" ]
then
  cp $CLIENT_LOG $CLIENT_TESTLOG
  printf "%-32s TEST OK\n" "$TEST_NAME" | tee $CLIENT_TESTLOG;
else
  echo "==== $CLIENT_LOG ====" ; cat $CLIENT_LOG;
  echo "==== $CLIENT_ERR ====" ; cat $CLIENT_ERR;
  echo "==== $MOSQ_SUB_LOG ====" ; cat $MOSQ_SUB_LOG;
  echo "==== $MOSQ_SUB_ERR ====" ; cat $MOSQ_SUB_ERR;

  printf "%-32s TEST FAIL\n" "$TEST_NAME" | tee $CLIENT_TESTLOG;
  rm -f $CLIENT_LOG $CLIENT_ERR $MOSQ_SUB_LOG $MOSQ_SUB_ERR mosquitto.conf
  exit 1
fi

rm -f $CLIENT_LOG $CLIENT_ERR $MOSQ_SUB_LOG $MOSQ_SUB_ERR mosquitto.conf
