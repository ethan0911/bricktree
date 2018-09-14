#!/bin/bash

OSPRAY_BIN=$1
BRICKTREEFILE=$2

COMMAND=$(basename "${OSPRAY_BIN}")
#echo $COMMAND
OUTPUTNAME=$(dirname "$BRICKTREEFILE")
#echo $OUTPUTNAME
#echo $(basename "$OUTPUTNAME")

#echo "${@:3}"

if [ $COMMAND == 'ospBrickWidget' ]
then
  $OSPRAY_BIN $BRICKTREEFILE -valueRange 0.0 230.0 \
    -vp -394.887 -421.328 -111.906 \
    -vi 543.59 552.347 978.952 \
    -vu 0.43045 0.457097 -0.778315 \
  "${@:3}"
fi


if [ $COMMAND == 'ospBrickBench' ]
then
  $OSPRAY_BIN $BRICKTREEFILE -valueRange 00.0187 1.5 \
    -vp -43.8931 -187.268 -663.12 \
    -vi 690.953 717.998 624.998 \
    -vu 0.495133 0.552345 -0.670641 \
    -o $OUTPUTNAME \
  "${@:3}"
fi


#$OSPRAY $BRICKTREEFILE \
#    -valueRange 0 1.5 \
#    -vp -175.826 1054.07 -178.418 \
#    -vi 256 256 256 \
#    -vu 0.00315318 0.160328 -1.00213 \
#  "${@:3}"





