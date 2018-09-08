#!/bin/bash

OSPRAY_BIN=$1
BRICKTREEFILE=$2

COMMAND=$(basename "${OSPRAY_BIN}")
#echo $COMMAND
OUTPUTNAME=$(dirname "$BRICKTREEFILE")
echo $OUTPUTNAME
echo $(basename "$OUTPUTNAME")

echo "${@:3}"

if [ $COMMAND == 'ospBrickWidget' ]
then
  $OSPRAY_BIN $BRICKTREEFILE -valueRange 0.0187 1.5 \
    -vp -175.826 1054.07 -178.418 \
    -vi 256 256 256 \
    -vu 0.286671 -0.333715 -0.898028 \
  "${@:3}"
fi


if [ $COMMAND == 'ospBrickBench' ]
then
  $OSPRAY_BIN $BRICKTREEFILE -valueRange 00.0187 1.5 \
    -vp -175.826 1054.07 -178.418 \
    -vi 256 256 256 \
    -vu 0.286671 -0.333715 -0.898028 \
    -o $OUTPUTNAME \
  "${@:3}"
fi


#$OSPRAY $BRICKTREEFILE \
#    -valueRange 0 1.5 \
#    -vp -175.826 1054.07 -178.418 \
#    -vi 256 256 256 \
#    -vu 0.00315318 0.160328 -1.00213 \
#  "${@:3}"





