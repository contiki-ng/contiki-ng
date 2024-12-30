#!/bin/bash

# Test basename
BASENAME=09-lwm2m-qmode-standalone-test

echo "Downloading leshan with Q-Mode support"
LESHAN_JAR=leshan-server-demo-qmode-support1.0.0-SNAPSHOT-jar-with-dependencies.jar
wget -nv -nc https://carlosgp143.github.io/resources/$LESHAN_JAR
echo "Starting leshan server with Q-Mode enabled"
java --add-opens java.base/java.util=ALL-UNNAMED -jar $LESHAN_JAR -lp 5686 -slp 5687 >leshan.log 2>leshan.err &
LESHID=$!

echo "Starting lwm2m standalone example"
example-lwm2m-standalone/lwm2m/lwm2m-example coap://127.0.0.1:5686 &

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
pgrep lwm2m-example | sudo xargs kill -9

sleep 1
echo "Closing leshan"
pgrep java | sudo xargs kill -9

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
