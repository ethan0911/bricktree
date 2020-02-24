#!/bin/bash

OSPRAY_BIN=$1
BRICKTREEFILE=$2

COMMAND=$(basename "${OSPRAY_BIN}")
#echo $COMMAND
OUTPUTNAME=$(dirname "$BRICKTREEFILE")
#echo $OUTPUTNAME
#echo $(basename "$OUTPUTNAME")



if [ $COMMAND == 'ospBrickWidget' ]
then
  $OSPRAY_BIN $BRICKTREEFILE -valueRange 250.0 550.0 \
    -vp -1849.73 -1813.13 -678.876 -vi 94.8442 162.559 640.049 -vu 1.37142 1.17118 -3.77636 \
  "${@:3}"
fi


if [ $COMMAND == 'ospBrickBench' ]
then
  $OSPRAY_BIN $BRICKTREEFILE -valueRange 0.0187 1.5 \
    -vp 440.651 619.864 155.128 -vi 494.944 248.239 259.144 -vu -0.0116661 -0.144736 -0.51102 \
    -o $OUTPUTNAME \
  "${@:3}"
fi


#$OSPRAY $BRICKTREEFILE \
#    -valueRange 0 1.5 \
#    -vp -175.826 1054.07 -178.418 \
#    -vi 256 256 256 \
#    -vu 0.00315318 0.160328 -1.00213 \
#  "${@:3}"





