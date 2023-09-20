#!/bin/bash
source ../utils.sh

# Contiki directory
CONTIKI=$1
# Test basename
BASENAME=$(basename $0 .sh)

IPADDR=fd00::302:304:506:708

declare -i OKCOUNT=0
declare -i TESTCOUNT=0

# Starting Contiki-NG native node
echo "Starting native CoAP server"
make -j4 -C $CONTIKI/examples/coap/coap-example-server || exit 1
sudo $CONTIKI/examples/coap/coap-example-server/coap-example-server.native &
CPID=$!
sleep 2

# Send CoAP requests
echo "Sending CoAP requests"

rm -f $BASENAME.log
for TARGET in .well-known/core test/push; do
  echo "Get $TARGET" | tee -a $BASENAME.log
  coap-client -v6 -m get coap://[$IPADDR]/$TARGET 2>&1 | tee coap.log
  cat coap.log >> $BASENAME.log
  # Fetch coap status code (not $? because this is piped)
  SUCCESS=`grep -c '2.05' coap.log`
  if [ $SUCCESS > 0 ]; then
    printf "> OK\n"
    OKCOUNT+=1
  else
    printf "> FAIL\n"
  fi
  TESTCOUNT+=1
done

echo "Closing native node"
sleep 2
kill_bg $CPID

if [ $TESTCOUNT -eq $OKCOUNT ] ; then
  printf "%-32s TEST OK    %3d/%d\n" "$BASENAME" "$OKCOUNT" "$TESTCOUNT" | tee $BASENAME.testlog;
else
  echo "==== coap.log ====" ; cat coap.log;
  echo "==== $BASENAME.log ====" ; cat $BASENAME.log;
  printf "%-32s TEST FAIL  %3d/%d\n" "$BASENAME" "$OKCOUNT" "$TESTCOUNT" | tee $BASENAME.testlog;
  rm -f coap.log
  exit 1
fi

rm -f coap.log
