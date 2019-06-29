#!/bin/sh

TEST_CODE_DIR=code-test-lc
TESTNAME=$1
TARGET=`echo ${TESTNAME} | sed -e 's/[0-9]*-\(.*\)/\1/'`

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
