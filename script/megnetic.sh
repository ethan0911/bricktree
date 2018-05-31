#!/bin/bash

OSPRAY=$1
BRICKTREEFILE=$2

echo ${@:2} 

$OSPRAY $BRICKTREEFILE \
  -valueRange 0 1.5 -vp 819.971 691.151 422.003 -vi 0 0 0 -vu -0.0141113 0.949951 -0.0297289 \
  "${@:3}"





