#!/bin/sh -e

TESTNAME=03-test-memb
TEST_CODE_DIR=code-test-memb
TARGET=test-memb

make -C ${TEST_CODE_DIR} clean
make -j4 -C ${TEST_CODE_DIR} ${TARGET}
${TEST_CODE_DIR}/${TARGET}
