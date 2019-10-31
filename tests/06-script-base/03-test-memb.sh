#!/bin/sh

TESTNAME=03-test-memb
TEST_CODE_DIR=code-test-memb
TARGET=test-memb

make -C ${TEST_CODE_DIR} clean
make -C ${TEST_CODE_DIR} ${TARGET}
${TEST_CODE_DIR}/${TARGET} > ${TESTNAME}.log

if [ $? -eq 0 ]; then
    echo "${TESTNAME} TEST OK" > ${TESTNAME}.testlog
    make -C ${TEST_CODE_DIR} clean
    exit 0
else
    echo "${TESTNAME} TEST FAIL" > ${TESTNAME}.testlog
    exit 1
fi
