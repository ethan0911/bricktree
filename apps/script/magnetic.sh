#!/bin/bash

OSPRAY_BIN=$1
BRICKTREEFILE=$2

COMMAND=$(basename "${OSPRAY_BIN}")
#echo $COMMAND
OUTPUTNAME=$(dirname "$BRICKTREEFILE")
#echo $OUTPUTNAME
#echo $(basename "$OUTPUTNAME")

#echo "${@:3}"

#Initial viewport
#-vp -175.826 1054.07 -178.418 -vi 256 256 256 -vu 0.286671 -0.333715 -0.898028 \

#Fig 7 viewport
#-vp 369.593 967.481 118.113 -vi 256 256 256 -vu -0.017369 -0.13602 -0.716157 
#-vp 440.651 619.864 155.128 -vi 494.944 248.239 259.144 -vu -0.0116661 -0.144736 -0.51102


if [ $COMMAND == 'ospBrickWidget' ]
then
  $OSPRAY_BIN $BRICKTREEFILE -valueRange 0.0187 1.5 \
    -vp 369.593 967.481 118.113 -vi 256 256 256 -vu -0.017369 -0.13602 -0.716157 \
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





