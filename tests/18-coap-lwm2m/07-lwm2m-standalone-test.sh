#!/bin/bash
source ../utils.sh

# Test basename
BASENAME=07-lwm2m-standalone-test

echo "Downloading leshan"
LESHAN_JAR=leshan-demo-server-2.0.0-SNAPSHOT-jar-with-dependencies.jar
wget -nv -nc https://joakimeriksson.github.io/resources/$LESHAN_JAR
echo "Starting leshan server"
java --add-opens java.base/java.util=ALL-UNNAMED -jar $LESHAN_JAR -lp 5686 -slp 5687 >leshan.log 2>leshan.err &
LESHID=$!

echo "Starting lwm2m standalone example"
example-lwm2m-standalone/lwm2m/lwm2m-example coap://127.0.0.1:5686 &

CPID=$!

COUNTER=10
while [ $COUNTER -gt 0 ]; do
    sleep 5
    if grep -q 'OK' leshan.err ; then
        echo OK with $COUNTER
        break
    fi
    let COUNTER-=1
done

echo "Closing standalone example"
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
