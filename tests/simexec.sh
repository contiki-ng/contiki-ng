#!/bin/bash
# The simulation to run
CSC=$1
shift
#Contiki directory
CONTIKI=$1
shift
#The basename of the experiment
BASENAME=$1
shift
#The random seed to start from
BASESEED=$1
shift
#The number of runs (with different seeds)
RUNCOUNT=$1
shift

# Counts all tests run
declare -i TESTCOUNT=0

# Counts successfull tests
declare -i OKCOUNT=0

# A list of seeds the resulted in failure
FAILSEEDS=

for (( SEED=$BASESEED; SEED<$(($BASESEED+$RUNCOUNT)); SEED++ )); do
  if java -Xshare:on -Dnashorn.args=--no-deprecation-warning -jar $CONTIKI/tools/cooja/dist/cooja.jar -nogui=$CSC -contiki=$CONTIKI -random-seed=$SEED; then
    OKCOUNT+=1
  else
    FAILSEEDS+=" $BASESEED"
  fi
  TESTCOUNT+=1
done

if [ $TESTCOUNT -ne $OKCOUNT ] ; then
	# At least one test failed
	printf "%-40s TEST FAIL  %3d/%d -- failed seeds:%s\n" "$BASENAME" "$OKCOUNT" "$TESTCOUNT" "$FAILSEEDS" > $BASENAME.testlog;
else
	printf "%-40s TEST OK    %3d/%d\n" "$BASENAME" "$OKCOUNT" "$TESTCOUNT" > $BASENAME.testlog;
fi

# We do not want Make to stop -> Return 0
# The Makefile will check if a log contains FAIL at the end
exit 0
