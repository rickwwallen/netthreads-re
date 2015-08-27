#!/bin/bash

########## Please adjust the variables below if needed

BDIR=.
PROJECT=`egrep "TARGET=" ${BDIR}/Makefile | tr -d "TARGET="`

LOADER=../../../loader/loader
BITFILE=../../../bit/netthreads-re.bit


########## Script should be ok from here on

INSTR=${BDIR}/${PROJECT}.instr.enc
HCODE=${BDIR}/${PROJECT}.hcode
DATA=${BDIR}/${PROJECT}.data.mif


if [  ! -f ${LOADER} -o  ! -f ${BITFILE} ]
then
    echo ${LOADER} or ${BITFILE} not found
    exit
fi

if [ ! -f ${INSTR} -o ! -f ${HCODE} -o ! -f ${DATA} ]
then
    echo ${INSTR} or ${HCODE} or ${DATA} not found
    exit
fi

nf2_download  ${BITFILE}
sleep 3



${LOADER} -h ${HCODE}

${LOADER} -i ${INSTR}
 
${LOADER} -d ${DATA}  -nodebug

