#!/bin/bash

# Contiki directory
CONTIKI=$1
# Test basename
BASENAME=08-lwm2m-qmode-ipso-test

IPADDR=fd00::302:304:506:708

# Starting Contiki-NG native node
echo "Starting native node - lwm2m/ipso objects with Q-Mode"
make -C $CONTIKI/examples/lwm2m-ipso-objects clean >/dev/null
make -C $CONTIKI/examples/lwm2m-ipso-objects DEFINES=LWM2M_QUEUE_MODE_CONF_ENABLED=1,LWM2M_QUEUE_MODE_CONF_INCLUDE_DYNAMIC_ADAPTATION=1,LWM2M_QUEUE_MODE_OBJECT_CONF_ENABLED=1 > make.log 2> make.err
sudo $CONTIKI/examples/lwm2m-ipso-objects/example-ipso-objects.native > node.log 2> node.err &
CPID=$!
sleep 10

echo "Downloading leshan with Q-Mode support"
LESHAN_JAR=leshan-server-demo-qmode-support1.0.0-SNAPSHOT-jar-with-dependencies.jar
wget -nc https://carlosgp143.github.io/resources/$LESHAN_JAR
echo "Starting leshan server with Q-Mode enabled"
java -jar $LESHAN_JAR >leshan.log 2>leshan.err &
LESHID=$!

COUNTER=10
while [ $COUNTER -gt 0 ]; do
    sleep 5
	aux=$(grep -c 'OK' leshan.err)
	if [ $aux -eq 2 ] ; then
        break
    fi
    let COUNTER-=1
done

echo "Closing native node"
sleep 1
pgrep ipso | sudo xargs kill -9

echo "Closing leshan"
sleep 1
pgrep java | sudo xargs kill -9

#Two OKs needed: awake and sleeping
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
