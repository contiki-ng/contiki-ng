#!/bin/sh

TESTNAME=04-test-result-visualization

CONTIKI=../..

TEST_CODE_DIR=code-result-visualization

make -C ${TEST_CODE_DIR} clean
make -C ${TEST_CODE_DIR}

${CONTIKI}/examples/benchmarks/result-visualization/run-analysis.py ${TEST_CODE_DIR}/COOJA.testlog > analysis.log || exit 1

# check that some packets were sent and all were received
grep "PDR=100" analysis.log > /dev/null || exit 1

# check that PDF files are created
[ -f plot_charge.pdf ] || exit 1
[ -f plot_duty_cycle_joined.pdf ] || exit 1
[ -f plot_duty_cycle.pdf ] || exit 1
[ -f plot_par.pdf ] || exit 1
[ -f plot_pdr.pdf ] || exit 1
[ -f plot_rpl_switches.pdf ] || exit 1

echo "${TESTNAME} TEST OK" > ${TESTNAME}.testlog

# success
exit 0
