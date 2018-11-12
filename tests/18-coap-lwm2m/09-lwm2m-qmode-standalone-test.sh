#!/bin/bash

# Contiki directory
CONTIKI=$1
# Test basename
BASENAME=09-lwm2m-qmode-standalone-test

# Building standalone posix example
echo "Compiling standalone posix example"
make CONTIKI_NG=../../$CONTIKI -C example-lwm2m-standalone/lwm2m clean >/dev/null
make CONTIKI_NG=../../$CONTIKI -C example-lwm2m-standalone/lwm2m DEFINES=LWM2M_QUEUE_MODE_CONF_ENABLED=1,LWM2M_QUEUE_MODE_CONF_INCLUDE_DYNAMIC_ADAPTATION=1,LWM2M_QUEUE_MODE_OBJECT_CONF_ENABLED=1 >make.log 2>make.err

echo "Downloading leshan with Q-Mode support"
LESHAN_JAR=leshan-server-demo-qmode-support1.0.0-SNAPSHOT-jar-with-dependencies.jar
wget -nc https://carlosgp143.github.io/resources/$LESHAN_JAR
echo "Starting leshan server with Q-Mode enabled"
java -jar $LESHAN_JAR -lp 5686 -slp 5687 >leshan.log 2>leshan.err &
LESHID=$!

echo "Starting lwm2m standalone example"
example-lwm2m-standalone/lwm2m/lwm2m-example coap://127.0.0.1:5686 > node.log 2> node.err &

CPID=$!

COUNTER=10
while [ $COUNTER -gt 0 ]; do
    sleep 5
	aux=$(grep -c 'OK' leshan.err)
	if [ $aux -eq 2 ] ; then
        break
    fi
    let COUNTER-=1
done

echo "Closing standalone example"
sleep 1
pgrep lwm2m-example | sudo xargs kill -9

echo "Closing leshan"
sleep 1
pgrep java | sudo xargs kill -9

aux=$(grep -c 'OK' leshan.err)
if [ $aux -eq 2 ]
then
  cp leshan.err $BASENAME.testlog;
  printf "%-32s TEST OK\n" "$BASENAME" | tee $BASENAME.testlog;
else
  echo "==== make.log ====" ; cat make.log;
  echo "==== make.err ====" ; cat make.err;
  echo "==== node.log ====" ; cat node.log;
  echo "==== node.err ====" ; cat node.err;
  echo "==== leshan.log ====" ; cat leshan.log;
  echo "==== leshan.err ====" ; cat leshan.err;
  echo "==== $BASENAME.log ====" ; cat $BASENAME.log;

  printf "%-32s TEST FAIL\n" "$BASENAME" | tee $BASENAME.testlog;
fi

rm make.log
rm make.err
rm node.log
rm node.err
rm leshan.log
rm leshan.err

# We do not want Make to stop -> Return 0
# The Makefile will check if a log contains FAIL at the end
exit 0
