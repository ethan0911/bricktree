#!/bin/bash


OSPRAY_BIN=$1
BRICKTREEFILE=$2

COMMAND=$(basename "${OSPRAY_BIN}")
#echo $COMMAND
OUTPUTNAME=$(dirname "$BRICKTREEFILE")
#echo $(basename "$OUTPUTNAME")

#echo "${@:3}"

if [ $COMMAND == 'ospBrickWidget' ]
then
  vglrun $OSPRAY_BIN $BRICKTREEFILE -valueRange -0.232 1.35 \
    -vp 11771.2 -3205.38 -493.458 -vi 5655.77 3801.46 1313.69 -vu -0.0931874 0.100636 -0.705542 \
    "${@:3}"
fi


# fig 7 
#-vp -3447.5 -1741.46 1651.07 -vi 7335.63 1203.53 -3.60962 -vu 0.166831 0.0520461 1.17983 



if [ $COMMAND == 'ospBrickBench' ]
then
  $OSPRAY_BIN $BRICKTREEFILE -valueRange -0.232 1.35 \
    -vp 11771.2 -3205.38 -493.458 -vi 5655.77 3801.46 1313.69 -vu -0.0931874 0.100636 -0.705542 -o $(basename "$OUTPUTNAME") ${@:3} 
fi
