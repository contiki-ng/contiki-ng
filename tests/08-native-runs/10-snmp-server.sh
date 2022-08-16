#!/bin/bash
source ../utils.sh

# Contiki directory
CONTIKI=$1
# Test basename
BASENAME=$(basename $0 .sh)

IPADDR=fd00::302:304:506:708

# Starting Contiki-NG native node
make -j4 -C $CONTIKI/examples/snmp-server
sudo $CONTIKI/examples/snmp-server/snmp-server.native &
CPID=$!

test_handler () {
  sleep 2
  $1 2>&1 | tee $BASENAME.log
  grep -z -E "$2" $BASENAME.log
  STATUS=$?

  if [ $STATUS -eq 0 ] ; then
    printf "%-32s TEST OK\n" "$BASENAME" | tee $BASENAME.testlog;
  else
    kill_bg $CPID
    echo $1
    echo $2
    printf "%-32s TEST FAIL\n" "$BASENAME" | tee $BASENAME.testlog;
    exit 1
  fi
}

# v1
## snmpget - pass
test_handler "snmpget -t2 -v1 -c public udp6:[$IPADDR]:161 1.3.6.1.2.1.1.1.0" "iso\.3\.6\.1\.2\.1\.1\.1\.0"
## snmpwalk - pass
test_handler "snmpwalk -t2 -v1 -c public udp6:[$IPADDR]:161 1" "iso\.3\.6\.1\.2\.1\.1\.1\.0.*iso\.3\.6\.1\.2\.1\.1\.2\.0.*iso\.3\.6\.1\.2\.1\.1\.3\.0.*iso\.3\.6\.1\.2\.1\.1\.4\.0.*iso\.3\.6\.1\.2\.1\.1\.5\.0.*iso\.3\.6\.1\.2\.1\.1\.6\.0.*iso\.3\.6\.1\.2\.1\.1\.7\.0.*End of MIB"

## snmpget - fail - noSuchName
test_handler "snmpget -t2 -v1 -c public udp6:[$IPADDR]:161 1.3.6.1.2.1.1.1" ".*noSuchName.*"
## snmpget - fail - timeout - 16 Ids in OID
test_handler "snmpget -t2 -v1 -c public udp6:[$IPADDR]:161 1.3.6.1.2.1.1.1.1.3.6.1.2.1.1.1.1" "Timeout.*"
## snmpget - fail - timeout - 4 OIDs
test_handler "snmpget -t2 -v1 -c public udp6:[$IPADDR]:161 1.3.6.1.2.1.1.1.0 1.3.6.1.2.1.1.1.0 1.3.6.1.2.1.1.1.0 1.3.6.1.2.1.1.1.0" "Timeout.*"

# v2
## snmpget - pass
test_handler "snmpget -t2 -v2c -c public udp6:[$IPADDR]:161 1.3.6.1.2.1.1.1.0" "iso\.3\.6\.1\.2\.1\.1\.1\.0"
## snmpwalk - pass
test_handler "snmpwalk -t2 -v2c -c public udp6:[$IPADDR]:161 1" "iso\.3\.6\.1\.2\.1\.1\.1\.0.*iso\.3\.6\.1\.2\.1\.1\.2\.0.*iso\.3\.6\.1\.2\.1\.1\.3\.0.*iso\.3\.6\.1\.2\.1\.1\.4\.0.*iso\.3\.6\.1\.2\.1\.1\.5\.0.*iso\.3\.6\.1\.2\.1\.1\.6\.0.*iso\.3\.6\.1\.2\.1\.1\.7\.0.*iso\.3\.6\.1\.2\.1\.1\.7\.0"
## snmpbulkget two non-repeater - pass
test_handler "snmpbulkget -v2c -Cn2 -c public udp6:[$IPADDR]:161 1 1" "iso\.3\.6\.1\.2\.1\.1\.1\.0.*iso\.3\.6\.1\.2\.1\.1\.1\.0"
## snmpbulkget two max-repetitions - pass
test_handler "snmpbulkget -t2 -v2c -Cr2 -c public udp6:[$IPADDR]:161 1" "iso\.3\.6\.1\.2\.1\.1\.1\.0.*iso\.3\.6\.1\.2\.1\.1\.2\.0"
## snmpbulkget one non-repeater and two max-repetitions - pass
test_handler "snmpbulkget -t2 -v2c -Cn1 -Cr2 -c public udp6:[$IPADDR]:161 1 1" "iso\.3\.6\.1\.2\.1\.1\.1\.0.*iso\.3\.6\.1\.2\.1\.1\.1\.0.*iso\.3\.6\.1\.2\.1\.1\.2\.0"

## snmpget - fail - noSuchName
test_handler "snmpget -t2 -v2c -c public udp6:[$IPADDR]:161 1.3.6.1.2.1.1.1" ".*No Such Instance currently.*"
## snmpget - fail - timeout - 16 Ids in OID
test_handler "snmpget -t2 -v2c -c public udp6:[$IPADDR]:161 1.3.6.1.2.1.1.1.1.3.6.1.2.1.1.1.1" "Timeout.*"
## snmpget - fail - timeout - 4 OIDs
test_handler "snmpget -t2 -v2c -c public udp6:[$IPADDR]:161 1.3.6.1.2.1.1.1.0 1.3.6.1.2.1.1.1.0 1.3.6.1.2.1.1.1.0 1.3.6.1.2.1.1.1.0" "Timeout.*"

#v3
## snmpget - fail - timeout - v3 not implemented
test_handler "snmpget -t2 -v3 -l authPriv -u snmp-poller -a SHA -A \"PASSWORD1\" -x AES -X \"PASSWORD1\" udp6:[$IPADDR]:161 1.3.6.1.2.1.1.1.0" ".*Timeout.*"

kill_bg $CPID > /dev/null 2>&1
wait $CPID > /dev/null 2>&1

# We do not want Make to stop -> Return 0
# The Makefile will check if a log contains FAIL at the end
exit 0
