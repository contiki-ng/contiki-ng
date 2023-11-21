#!/usr/bin/env bash

CNT_SUCCESS=0
CNT_ALL=0

for DIR in $(find examples/ -type d -exec test -f '{}'/Makefile \; -print -prune); do
  echo $DIR && ((CNT_ALL++))
  make -C $DIR -j$(nproc) && ((CNT_SUCCESS++))
  make -C $DIR clean
done

echo  Succeeded in compiling $CNT_SUCCESS/$CNT_ALL apps
