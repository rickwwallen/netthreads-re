#!/bin/bash

if [ $# -ne 1 ]
then
    echo enter name of benchmark
    exit
fi
TRACE=/tmp/mint_gprof.txt
bench=../src/bench/$1/$1.map



if [ ! -f ${TRACE} ]
then
    echo did not find ${TRACE}
    exit
fi

if [ ! -f ${bench} ]
then
    echo did not find $bench
    exit
fi

awk 'BEGIN{sp=1;}/JAL/{pc[sp]=$2; time[sp]=$NF; sp=sp+1;}/RET/{ indent=index($0,"R"); for(i=0; i<indent; i++) printf(" "); sp=sp-1;print time[sp]" "pc[sp]" "$NF-time[sp]; if(sp <= 0) {print "WARNING sp is "sp; sp=1;}}{ next;}' ${TRACE} > /tmp/2.txt

awk 'NF==2{sub("0x00000000","",$0);print; }{next;}' $bench  | egrep '                2' > /tmp/1.txt

awk '{print "s/"$1"/"$2"/"}' /tmp/1.txt > /tmp/3.txt

sed -f /tmp/3.txt /tmp/2.txt > /tmp/4.txt

sort -g /tmp/4.txt | perl -pne 's/[0-9]+ //' > /tmp/5.txt

echo RESULT IS IN /tmp/5.txt
