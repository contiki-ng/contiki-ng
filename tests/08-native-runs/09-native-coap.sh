#!/bin/bash
source ../utils.sh

# Contiki directory
CONTIKI=../..
# Test basename
BASENAME=$(basename $0 .sh)

IPADDR=fd00::302:304:506:708

# Distributions differ on the name: Debian and Ubuntu ship the client once per
# TLS backend and provide no unsuffixed name, while a source build installs
# coap-client itself. A client under any other name can be given in
# COAP_CLIENT.
if [ -z "$COAP_CLIENT" ]; then
  for CANDIDATE in coap-client coap-client-notls coap-client-openssl \
                   coap-client-gnutls coap-client-mbedtls; do
    if command -v $CANDIDATE > /dev/null; then
      COAP_CLIENT=$CANDIDATE
      break
    fi
  done
fi
if [ -z "$COAP_CLIENT" ] || ! command -v "$COAP_CLIENT" > /dev/null; then
  echo "No CoAP client found, install libcoap or set COAP_CLIENT"
  printf "%-32s TEST FAIL\n" "$BASENAME" | tee $BASENAME.testlog
  exit 1
fi

declare -i OKCOUNT=0
declare -i TESTCOUNT=0

# Starting Contiki-NG native node
echo "Starting native CoAP server"
sudo $CONTIKI/examples/coap/coap-example-server/build/native/coap-example-server.native &
CPID=$!
sleep 2

# Send CoAP requests
echo "Sending CoAP requests"

rm -f $BASENAME.log coap.log
for TARGET in .well-known/core test/push; do
  echo "Get $TARGET" | tee -a $BASENAME.log
  "$COAP_CLIENT" -v6 -m get coap://[$IPADDR]/$TARGET 2>&1 | tee coap.log
  cat coap.log >> $BASENAME.log
  # Fetch coap status code (not $? because this is piped)
  SUCCESS=`grep -c '2.05' coap.log`
  if [ "${SUCCESS:-0}" -gt 0 ]; then
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
