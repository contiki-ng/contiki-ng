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
#The basename of the experiment
RUNCOUNT=$1
shift

# Counts all tests run
declare -i TESTCOUNT=0

# Counts successfull tests
declare -i OKCOUNT=0

for (( SEED=1; SEED<=$RUNCOUNT; SEED++ )); do
	echo -n "Running test $BASENAME with random Seed $SEED"

	# run simulation
	java -Xshare:on -jar $CONTIKI/tools/cooja/dist/cooja.jar -nogui=$CSC -contiki=$CONTIKI -random-seed=$SEED > $BASENAME.log &
	JPID=$!

	# Copy the log and only print "." if it changed
	touch $BASENAME.log.prog
	while kill -0 $JPID 2> /dev/null
	do
		sleep 1
		diff $BASENAME.log $BASENAME.log.prog > /dev/null
		if [ $? -ne 0 ]
		then
		  echo -n "."
		  cp $BASENAME.log $BASENAME.log.prog
		fi
	done
	rm $BASENAME.log.prog

  # wait for end of simulation
	wait $JPID
	JRV=$?

  TESTCOUNT+=1
	if [ $JRV -eq 0 ] ; then
		touch COOJA.testlog;
		mv COOJA.testlog $BASENAME.testlog
		OKCOUNT+=1
		echo " OK"
	else
		# Verbose output when using CI
		if [ "$CI" = "true" ]; then
			echo "==== $BASENAME.log ====" ; cat $BASENAME.log;
			echo "==== COOJA.testlog ====" ; cat COOJA.testlog;
			echo "==== Files used for simulation (sha1sum) ===="
			grep "Loading firmware from:" COOJA.log | cut -d " " -f 10 | uniq  | xargs -r sha1sum
			grep "Creating core communicator between Java class" COOJA.log | cut -d " " -f 17 | uniq  | xargs -r sha1sum
		else
			tail -50 $BASENAME.log ;
		fi;

		mv COOJA.testlog $BASENAME.$SEED.faillog
		echo " FAIL ಠ_ಠ" | tee -a $BASENAME.$SEED.faillog;
	fi

	shift
done

echo "Test $BASENAME, successfull runs: $OKCOUNT/$TESTCOUNT"

if [ $TESTCOUNT -ne $OKCOUNT ] ; then
	# At least one test failed
	touch COOJA.testlog;
	mv COOJA.testlog $BASENAME.testlog;
fi

# We do not want Make to stop -> Return 0
# The Makefile will check if a log contains FAIL at the end
exit 0
