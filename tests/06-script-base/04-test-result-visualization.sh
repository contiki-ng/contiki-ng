#!/bin/sh -e

../../examples/benchmarks/result-visualization/run-analysis.py code-result-visualization/COOJA.testlog > analysis.log

# check that some packets were sent and all were received
grep "PDR=100" analysis.log > /dev/null || exit 1

# check that PDF files are created
[ -f plot_charge.pdf ] || exit 1
[ -f plot_duty_cycle_joined.pdf ] || exit 1
[ -f plot_duty_cycle.pdf ] || exit 1
[ -f plot_par.pdf ] || exit 1
[ -f plot_pdr.pdf ] || exit 1
[ -f plot_rpl_switches.pdf ] || exit 1
