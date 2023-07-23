#!/bin/sh -e

TESTNAME=01-rpl-udp
TEST_CODE_DIR=../../examples/rpl-udp
TARGET=cc2538dk

make -C ${TEST_CODE_DIR} clean TARGET=${TARGET}
make -C ${TEST_CODE_DIR} TARGET=${TARGET}

renode-test --show-log ${TEST_CODE_DIR}/rpl-udp.robot

if [ $? -eq 0 ]; then
    echo "${TESTNAME} TEST OK" > ${TESTNAME}.testlog
    make -C ${TEST_CODE_DIR} clean TARGET=${TARGET}
    exit 0
else
    echo "${TESTNAME} TEST FAIL" > ${TESTNAME}.testlog
    exit 1
fi
