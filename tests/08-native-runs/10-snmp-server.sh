#!/bin/bash
source ../utils.sh

# Contiki directory
CONTIKI=$1
# Test basename
BASENAME=$(basename $0 .sh)

IPADDR=fd00::302:304:506:708

test_handler () {
  # Starting Contiki-NG native node
  make -C $CONTIKI/examples/snmp-server > make.log 2> make.err
  sudo $CONTIKI/examples/snmp-server/snmp-server.native > node.log 2> node.err &
  CPID=$!
  sleep 2

  $1 2>&1 | grep -z -E "$2" >> $BASENAME.log 2>&1 
  STATUS=$?
  
  sleep 2
  kill_bg $CPID > /dev/null 2>&1
  wait $CPID > /dev/null 2>&1

  if [ $STATUS -eq 0 ] ; then
    cp $BASENAME.log $BASENAME.testlog
    printf "%-32s TEST OK\n" "$BASENAME" | tee $BASENAME.testlog;
  else
    echo "==== make.log ====" ; cat make.log;
    echo "==== make.err ====" ; cat make.err;
    echo "==== node.log ====" ; cat node.log;
    echo "==== node.err ====" ; cat node.err;
    echo "==== $BASENAME.log ====" ; cat $BASENAME.log;
    
    printf "%-32s TEST FAIL\n" "$BASENAME" | tee $BASENAME.testlog;
  fi

  rm make.log
  rm make.err
  rm node.log
  rm node.err
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

# We do not want Make to stop -> Return 0
# The Makefile will check if a log contains FAIL at the end
exit 0
