#!/bin/bash
source ../utils.sh

# Contiki directory
CONTIKI=../..
# Test basename
BASENAME=10-lwm2m-ipso-dtls-test.sh

IPADDR=fd00::302:304:506:708

# Starting Contiki-NG native node
echo "Starting native node - lwm2m/ipso objects"
sudo $CONTIKI/examples/lwm2m-ipso-objects/build/native/example-ipso-objects.native &
CPID=$!

echo "Downloading leshan"
LESHAN_JAR=leshan-demo-server-2.0.0-SNAPSHOT-jar-with-dependencies.jar
wget -nv -nc https://joakimeriksson.github.io/resources/$LESHAN_JAR
sleep 10
echo "Starting leshan server"
java --add-opens java.base/java.util=ALL-UNNAMED -jar $LESHAN_JAR >leshan.log 2>leshan.err &
LESHID=$!

COUNTER=10
while [ $COUNTER -gt 0 ]; do
    sleep 5
    if grep -q 'OK' leshan.err ; then
        break
    fi
    let COUNTER-=1
done

echo "Closing native node"
kill_bg $CPID

sleep 1
echo "Closing leshan"
kill_bg $LESHID


if ! grep -q 'OK' leshan.err ; then
  echo "==== leshan.log ====" ; cat leshan.log;
  echo "==== leshan.err ====" ; cat leshan.err;
  printf "%-32s TEST FAIL\n" "$BASENAME" | tee $BASENAME.testlog;
  rm -f leshan.log leshan.err
  exit 1
fi

rm -f leshan.log leshan.err
