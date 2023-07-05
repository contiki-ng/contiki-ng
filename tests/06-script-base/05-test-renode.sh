#!/bin/sh -e

TESTNAME=05-test-renode
TEST_CODE_DIR=../../examples/rpl-udp
TARGET=cc2538dk

make -C ${TEST_CODE_DIR} clean TARGET=${TARGET}
make -C ${TEST_CODE_DIR} TARGET=${TARGET}

renode-test ${TEST_CODE_DIR}/rpl-udp.robot > ${TESTNAME}.log

if [ $? -eq 0 ]; then
    echo "${TESTNAME} TEST OK" > ${TESTNAME}.testlog
    make -C ${TEST_CODE_DIR} clean TARGET=${TARGET}
    exit 0
else
    echo "${TESTNAME} TEST FAIL" > ${TESTNAME}.testlog
    exit 1
fi
