#!/bin/bash
source ../utils.sh

# Contiki directory
CONTIKI=$1

# Example code directory
CODE_DIR=$CONTIKI/examples/mqtt-client/
CODE=mqtt-client

CLIENT_LOG=$CODE.log
CLIENT_TESTLOG=$CODE.testlog
CLIENT_ERR=$CODE.err
MOSQ_SUB_LOG=mosquitto_sub.log
MOSQ_SUB_ERR=mosquitto_sub.err

# Start mosquitto server
echo "Starting mosquitto daemon"
mosquitto &> /dev/null &
MOSQID=$!
sleep 2

# Start mosquitto_sub client. Subscribe
echo "Starting mosquitto subscriber"
mosquitto_sub -t iot-2/evt/status/fmt/json > $MOSQ_SUB_LOG 2> $MOSQ_SUB_ERR &
MSUBID=$!
sleep 2

# Starting Contiki-NG native node
echo "Starting native node"
make -C $CODE_DIR -B TARGET=native \
  DEFINES=MQTT_CLIENT_CONF_ORG_ID=\\\"travis-test\\\",MQTT_CLIENT_CONF_LOG_LEVEL=LOG_LEVEL_DBG \
  > make.log 2> make.err
sudo $CODE_DIR/$CODE.native > $CLIENT_LOG 2> $CLIENT_ERR &
CPID=$!

# The mqtt-client will publish every 30 secs. Wait for 45
sleep 45

# Send a publish to the mqtt client
mosquitto_pub -m "1" -t iot-2/cmd/leds/fmt/json

echo "Closing native node"
sleep 2
kill_bg $CPID

echo "Stopping mosquitto daemon"
kill_bg $MOSQID

echo "Stopping mosquitto subscriber"
kill_bg $MSUBID

# Success criteria:
# * mosquitto_sub output not empty
# * mqtt-client.native output contains "MQTT SUB"
SUB_RCV=`grep "MQTT SUB" $CLIENT_LOG`
if [ -s "$MOSQ_SUB_LOG" -a -n "$SUB_RCV" ]
then
  cp $CLIENT_LOG $CODE.testlog
  printf "%-32s TEST OK\n" "$CODE" | tee $CODE.testlog;
else
  echo "==== make.log ====" ; cat make.log;
  echo "==== make.err ====" ; cat make.err;
  echo "==== $CLIENT_LOG ====" ; cat $CLIENT_LOG;
  echo "==== $CLIENT_ERR ====" ; cat $CLIENT_ERR;
  echo "==== $MOSQ_SUB_LOG ====" ; cat $MOSQ_SUB_LOG;
  echo "==== $MOSQ_SUB_ERR ====" ; cat $MOSQ_SUB_ERR;

  printf "%-32s TEST FAIL\n" "$CODE" | tee $CODE.testlog;
fi

rm make.log
rm make.err
rm $CLIENT_LOG $CLIENT_ERR
rm $MOSQ_SUB_LOG $MOSQ_SUB_ERR

# We do not want Make to stop -> Return 0
# The Makefile will check if a log contains FAIL at the end
exit 0
