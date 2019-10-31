#!/bin/bash

function echo_run( )
{
    echo $@
    $@
}

# arg 1: PID
# arg 2 (optional): signal (default: 9)
function kill_bg( )
{
    PID=$1
    CMD=$(ps -p $PID -o cmd=)
    SUDO=
    TOKILL=$PID
    SIGNAL=${2:-9}
    if [[ ${CMD:0:5} == "sudo " ]] ; then
        SUDO="sudo "
        TOKILL=$(ps --ppid $PID -o pid=)
    fi
    echo_run ${SUDO}kill -$SIGNAL $TOKILL
}
