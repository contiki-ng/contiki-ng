#!/bin/bash

function echo_run( )
{
    echo $@
    $@
}

function kill_bg( )
{
    PID=$1
    CMD=$(ps -p $PID -o cmd=)
    SUDO=
    TOKILL=$PID
    if [[ ${CMD:0:5} == "sudo " ]] ; then
        SUDO="sudo "
        TOKILL=$(ps --ppid $PID -o pid=)
    fi
    echo_run ${SUDO}kill -9 $TOKILL
}
