#!/bin/bash

OSPRAY=$1
BRICKTREEFILE=$2

echo ${@:2} 

$OSPRAY $BRICKTREEFILE \
    -valueRange 0 1.5 \
    -vp -175.826 1054.07 -178.418 \
    -vi 256 256 256 \
    -vu 0.00315318 0.160328 -1.00213 \
  "${@:3}"





