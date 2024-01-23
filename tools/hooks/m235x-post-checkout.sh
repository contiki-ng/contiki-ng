#!/bin/bash

if [ "x$name" = "xarch/cpu/m2351/bsp" -o "x$name" = "xarch/cpu/m2354/bsp" ]; then
    library_dir=${toplevel}/${name}/Library
    STDDRIVER=${library_dir}/StdDriver/src
    echo ${STDDRIVER}
    [ -e ${STDDRIVER}/spi.c ] && echo ${STDDRIVER}/spi.c
    [ -e ${STDDRIVER}/spi.c ] && mv ${STDDRIVER}/spi.c ${STDDRIVER}/nu_spi.c
    [ -e ${STDDRIVER}/timer.c ] && echo ${STDDRIVER}/timer.c
    [ -e ${STDDRIVER}/timer.c ] && mv ${STDDRIVER}/timer.c ${STDDRIVER}/nu_timer.c
    SYSTEM_M2351_c="${library_dir}/Device/Nuvoton/M2351/Source/system_M2351.c"
    [ -e ${SYSTEM_M2351_c} -a "grep -q PRIx32 ${SYSTEM_M2351_c}" ] && patch -p1 < ${toplevel}/tools/hooks/system_M2351.patch
    SYSTEM_M2354_c="${library_dir}/Device/Nuvoton/M2354/Source/system_M2354.c"
    [ -e ${SYSTEM_M2354_c} -a "grep -q PRIx32 ${SYSTEM_M2354_c}" ] && patch -p1 < ${toplevel}/tools/hooks/system_M2354.patch
    echo -n ""
fi
