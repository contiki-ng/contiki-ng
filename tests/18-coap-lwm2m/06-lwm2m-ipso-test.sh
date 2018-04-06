#!/bin/bash

# Contiki directory
CONTIKI=$1
# Test basename
BASENAME=06-lwm2m-ipso-test

IPADDR=fd00::302:304:506:708

# Starting Contiki-NG native node
echo "Starting native node - lwm2m/ipso objects"
make -C $CONTIKI/examples/ipso-objects clean >/dev/null
make -C $CONTIKI/examples/ipso-objects > make.log 2> make.err
sudo $CONTIKI/examples/ipso-objects/example-ipso-objects.native > node.log 2> node.err &
CPID=$!
sleep 10

echo "Downloading leshan"
wget -nc https://joakimeriksson.github.io/resources/leshan-server-demo-1.0.0-SNAPSHOT-jar-with-dependencies.jar
echo "Starting leshan server"
java -jar leshan-server-demo-1.0.0-SNAPSHOT-jar-with-dependencies.jar >leshan.log 2>leshan.err &
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
sleep 1
pgrep ipso | sudo xargs kill -9

echo "Closing leshan"
sleep 1
pgrep java | sudo xargs kill -9


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
