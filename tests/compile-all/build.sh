#!/bin/bash

# Copyright (c) 2018, University of Bristol
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. Neither the name of the University nor the names of its contributors
#    may be used to endorse or promote products derived from this software
#    without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE UNIVERSITY AND CONTRIBUTORS ``AS IS'' AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
# OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.
#
# Author: Atis Elsts

#
# This file builds all examples for all platforms, excluding just those
# example and platform combinations that are marked as impossible in the Makefiles
# by using PLATFORMS_EXCLUDE and PLATFORMS_ONLY variables.
#
# This script can also clean all targets. To do that, run:
# ./build.sh clean
#
# To invoke the building for a specific platform, run:
# $ PLATFORMS=zoul ./build.sh
#
CONTIKI_NG_TOP_DIR="../.."
EXAMPLES_DIR=$CONTIKI_NG_TOP_DIR/examples

if [[ "$PLATFORMS" == "" ]]
then
    PLATFORMS=`ls $CONTIKI_NG_TOP_DIR/arch/platform`
fi

if [[ "$MAKEFILES" == "" ]]
then
    MAKEFILES=`find $EXAMPLES_DIR -name Makefile`
fi

HELLO_WORLD=$EXAMPLES_DIR/hello-world

# Set the make goal the first argument of the script or to "all" if called w/o arguments
if [[ $# -gt 0 ]]
then
    GOAL=$1
else
    GOAL="all"
fi

# Logging level:
# 0 - quiet
# 1 - normal; prints compilation and link messages only on errors
# 2 - print all compilation and link messages
LOG_LEVEL=1

if [[ $LOG_LEVEL -ge 1 ]]
then
    LOG_INFO=echo
    CAT_INFO=cat
else
    LOG_INFO=true
    CAT_INFO=true
fi

if [[ $LOG_LEVEL -ge 2 ]]
then
    LOG_DEBUG=echo
    CAT_DEBUG=cat
else
    LOG_DEBUG=true
    CAT_DEBUG=true
fi

NUM_SUCCESS=0
NUM_SKIPPED=0
NUM_FAILED=0

rm -f failed.log
rm -f failed-full.log
FAILED=

for platform in $PLATFORMS
do
    # Cooja is not very friendly for command line builds at the moment
    if [[ "$platform" == "cooja" ]]
    then
        $LOG_INFO "Skipping the Cooja platform"
        continue
    fi

    # Detect all boards for the current platform by calling
    # make TARGET=$platform boards
    # in the hello-world dir.
    BOARDS=`make -s -C $HELLO_WORLD TARGET=$platform boards \
            | grep -v "no boards" | rev | cut -f3- -d" " | rev`

    if [[ -z $BOARDS ]]
    then
        BOARDS="default"
    fi

    $LOG_INFO "====================================================="
    $LOG_INFO "Going through all examples for platform \"$platform\""
    $LOG_INFO "====================================================="
    for example in $MAKEFILES
    do

        for board in $BOARDS
        do
            example_dir=`dirname "$example"`

            # Clean it before building
            make -C "$example_dir" TARGET=$platform BOARD=$board clean 2>&1 >/dev/null
            if [[ "$GOAL" == "clean" ]]
            then
               # do this just for the first board
               break
            fi

            # Build the goal
            $LOG_INFO "make -C \"$example_dir\" -j TARGET=$platform BOARD=$board $GOAL"
            if make -C "$example_dir" -j TARGET=$platform BOARD=$board $GOAL >build.log 2>&1
            then
                $LOG_INFO "..done"
                $CAT_DEBUG build.log
                if [[ `grep Skipping build.log` ]]
                then
                    NUM_SKIPPED=$(($NUM_SKIPPED + 1))
                else
                    NUM_SUCCESS=$(($NUM_SUCCESS + 1))
                fi
            else
                $LOG_INFO "Failed to build $example_dir for $platform ($board)"
                $CAT_DEBUG build.log
                echo "TARGET=$platform BOARD=$board $example_dir" >> failed.log
                echo "TARGET=$platform BOARD=$board $example_dir" >> failed-full.log
                cat build.log >> failed-full.log
                echo "=====================" >> failed-full.log
                NUM_FAILED=$(($NUM_FAILED + 1))
                FAILED="$FAILED; $example_dir for $platform ($board)"
            fi

            # Clean it after building
            make -C "$example_dir" TARGET=$platform BOARD=$board clean 2>&1 >/dev/null
        done
    done
done

# If building, not cleaning, print so statistics
if [[ "$GOAL" == "all" ]]
then
    $LOG_INFO "Number of examples built successfully: $NUM_SUCCESS"
    $LOG_INFO "Number of examples skipped: $NUM_SKIPPED"
    $LOG_INFO "Number of examples that failed to build: $NUM_FAILED"
    $LOG_INFO "Failed examples: $FAILED"
fi
