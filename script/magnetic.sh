#!/bin/bash

OSPRAY=$1
BRICKTREEFILE=$2

echo ${@:2} 

$OSPRAY $BRICKTREEFILE \
    -valueRange 0 1.5 \
    -vp 800 800 800 \
    -vi 256 256 256 \
    -vu -0.0141113 0.949951 -0.0297289 \
  "${@:3}"





