#!/bin/sh -e

TESTNAME=01-rpl-udp
TEST_CODE_DIR=../../examples/rpl-udp
TARGET=cc2538dk

make -C ${TEST_CODE_DIR} clean TARGET=${TARGET}
make -C ${TEST_CODE_DIR} TARGET=${TARGET}
renode-test --show-log ${TEST_CODE_DIR}/rpl-udp.robot
