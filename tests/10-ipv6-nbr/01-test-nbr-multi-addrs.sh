#!/bin/sh -e

TEST_NAME=01-test-nbr-multi-addrs

if [ $# -eq 1 ]; then
    # Absolute path to CONTIKI_DIR in $1.
    TEST_DIR=$1/tests/10-ipv6-nbr
else
    TEST_DIR=.//tests/10-ipv6-nbr
fi
SRC_DIR=${TEST_DIR}/nbr-multi-addrs
EXEC_FILE_NAME=test.native

make -C ${SRC_DIR} clean
make -j4 -C ${SRC_DIR}
${SRC_DIR}/${EXEC_FILE_NAME}
