#!/bin/bash

# Contiki directory
CONTIKI=../..
# Test basename
BASENAME=08-lwm2m-qmode-ipso-test

IPADDR=fd00::302:304:506:708

# Starting Contiki-NG native node
echo "Starting native node - lwm2m/ipso objects with Q-Mode"
sudo $CONTIKI/examples/lwm2m-ipso-objects/build/native/example-ipso-objects.native &
CPID=$!

echo "Downloading leshan with Q-Mode support"
LESHAN_JAR=leshan-server-demo-qmode-support1.0.0-SNAPSHOT-jar-with-dependencies.jar
wget -nv -nc https://carlosgp143.github.io/resources/$LESHAN_JAR
sleep 10
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
pgrep ipso | sudo xargs kill -9

sleep 1
echo "Closing leshan"
pgrep java | sudo xargs kill -9

#Two OKs needed: awake and sleeping
aux=$(grep -c 'OK' leshan.err)
if [ $aux -eq 2 ]
then
  cp leshan.err $BASENAME.testlog;
  printf "%-32s TEST OK\n" "$BASENAME" | tee $BASENAME.testlog;
else
  echo "==== leshan.log ====" ; cat leshan.log;
  echo "==== leshan.err ====" ; cat leshan.err;
  printf "%-32s TEST FAIL\n" "$BASENAME" | tee $BASENAME.testlog;
  rm -f leshan.log leshan.err
  exit 1
fi

rm -f leshan.log leshan.err

