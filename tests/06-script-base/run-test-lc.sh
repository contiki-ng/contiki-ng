#!/bin/sh -e

TEST_CODE_DIR=code-test-lc
TESTNAME=$1
TARGET=`echo ${TESTNAME} | sed -e 's/[0-9]*-\(.*\)/\1/'`

make -C ${TEST_CODE_DIR} clean
make -j4 -C ${TEST_CODE_DIR} ${TARGET}
${TEST_CODE_DIR}/${TARGET}
