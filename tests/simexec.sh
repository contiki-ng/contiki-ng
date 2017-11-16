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
	echo -n "Running test $BASENAME with random Seed $SEED"

	# run simulation
	java -Xshare:on -jar $CONTIKI/tools/cooja/dist/cooja.jar -nogui=$CSC -contiki=$CONTIKI -random-seed=$SEED > $BASENAME.$SEED.coojalog &
	JPID=$!

	# Copy the log and only print "." if it changed
	touch progress.log
	while kill -0 $JPID 2> /dev/null
	do
		sleep 1
		diff $BASENAME.$SEED.coojalog progress.log > /dev/null
		if [ $? -ne 0 ]
		then
		  echo -n "."
		  cp $BASENAME.$SEED.coojalog progress.log
		fi
	done
	rm progress.log

  # wait for end of simulation
	wait $JPID
	JRV=$?

	# Save testlog
	touch COOJA.testlog;
	mv COOJA.testlog $BASENAME.$SEED.scriptlog
	rm COOJA.log

  TESTCOUNT+=1
	if [ $JRV -eq 0 ] ; then
		OKCOUNT+=1
		echo " OK"
	else
		FAILSEEDS+=" $BASESEED"
		echo " FAIL"
		echo "==== $BASENAME.$SEED.coojalog ====" ; cat $BASENAME.$SEED.coojalog;
		echo "==== $BASENAME.$SEED.scriptlog ====" ; cat $BASENAME.$SEED.scriptlog;
	fi
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
