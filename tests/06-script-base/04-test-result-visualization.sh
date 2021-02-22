#!/bin/sh

CONTIKI=../..

TEST_CODE_DIR=code-result-visualization

make -C ${TEST_CODE_DIR} clean
make -C ${TEST_CODE_DIR}

${CONTIKI}/examples/benchmarks/result-visualization/run-analysis.py ${TEST_CODE_DIR}/00-result-visualization.1.scriptlog > analysis.log || exit -1

# check that some packets were sent and all were received
grep "PDR=100" analysis.log || exit -1

# check that PDF files are created
ls plot_charge.pdf || exit -1
ls plot_duty_cycle_joined.pdf || exit -1
ls plot_duty_cycle.pdf || exit -1
ls plot_par.pdf || exit -1
ls plot_pdr.pdf || exit -1
ls plot_rpl_switches.pdf || exit -1

# success
exit 0
