#!/bin/bash
source ../utils.sh

# Contiki directory
CONTIKI=$1
# Test basename
BASENAME=07-lwm2m-standalone-test

# Building standalone posix example
echo "Compiling standalone posix example"
make CONTIKI_NG=../../$CONTIKI -C example-lwm2m-standalone/lwm2m clean >/dev/null
make CONTIKI_NG=../../$CONTIKI -C example-lwm2m-standalone/lwm2m >make.log 2>make.err

echo "Downloading leshan"
LESHAN_JAR=leshan-server-demo-1.0.0-SNAPSHOT-jar-with-dependencies.jar
wget -nc https://joakimeriksson.github.io/resources/$LESHAN_JAR
echo "Starting leshan server"
java -jar $LESHAN_JAR -lp 5686 -slp 5687 >leshan.log 2>leshan.err &
LESHID=$!

echo "Starting lwm2m standalone example"
example-lwm2m-standalone/lwm2m/lwm2m-example coap://127.0.0.1:5686 > node.log 2> node.err &

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
sleep 1
kill_bg $CPID

echo "Closing leshan"
sleep 1
kill_bg $LESHID


if grep -q 'OK' leshan.err ; then
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
