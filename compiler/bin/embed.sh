#!/bin/bash

NF_HOME=../../../compiler

str=`pwd`
DIR=$str
BENCH=`basename $str`


###### USE THIS FLAG TO RUN IN THE SIMULATOR IN DEBUGGING
###### MODE (i.e. with log() calls enabled)
#FLAG="CONTEXT=sim"
FLAG=

echo BENCH $BENCH  DIR $DIR

make clean
make ${FLAG}
make embed ${FLAG}




BB=${DIR}/${BENCH}
echo USING ${BB} 

if [ ! -f ${BB}.instr.mif ]
then
    echo ERROR file ${BB}.instr.mif is missing
    exit
fi


awk '/:/{print $3}' ${BB}.instr.mif | tr -d ";" > preproc.txt

make

CMD="${NF_HOME}/bin/hazard ${BB}"
$CMD  > ${BB}.haz

# now glue to hazard codes to the original opcodes


cat ${BB}.haz| cut -f 1  | paste -d "" - preproc.txt > z.txt

awk 'BEGIN {
print "WIDTH=36;";
print "DEPTH=65536;";
print "";
print "ADDRESS_RADIX=HEX;";
print "DATA_RADIX=HEX;";
print "";
print "CONTENT BEGIN";
print "";}
{ 
printf("%08x : %s;\n", NR-1, $1);
}
END {
print "END;"
}' z.txt > y.txt

mv y.txt ${BB}.instr.hmif
rm z.txt preproc.txt

echo done


docode ()
{
    if [ ! -f ${BB}.instr.mif ]
	then
	echo ERROR file ${BB}.instr.mif is missing
	exit
    fi


    awk '/:/{print $3}' ${BB}.instr.mif | tr -d ";" > preproc.txt
    awk '{print $1}' ${BB}.haz  > haz.txt

    make

    ${NF_HOME}/bin/recoding ${BB} > zim.txt

    echo -n `basename ${BB}`" "

    if [ -f conv.txt ]
	then
	echo "conversion done, result in ${BB}.instr.enc"
	mv conv.txt ${BB}.instr.enc
    else
	echo building dictionary, rerun to get conversion
    fi


}

rm -f ref.txt conv.txt

docode

cp dict.txt ref.txt

docode

awk '{printf("%x: %s\n", $1, $2);}' ref.txt > ${BB}.hcode

rm -f dict.txt haz.txt ref.txt preproc.txt tb.v zim.txt
echo COMPLETE	
