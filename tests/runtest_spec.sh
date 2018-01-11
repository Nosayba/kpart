#!/bin/bash

BUILD_DIR=/path/to/benchmarks/build/speccpu2006/429.mcf
INPUTS_DIR=/path/to/benchmarks/inputs/speccpu2006/429.mcf

mkdir -p p0
mkdir -p p1
mkdir -p p2
mkdir -p p3
mkdir -p p4
mkdir -p p5
mkdir -p p6
mkdir -p p7

cp ${INPUTS_DIR}/ref/* p0
cp ${INPUTS_DIR}/ref/* p1
cp ${INPUTS_DIR}/ref/* p2
cp ${INPUTS_DIR}/ref/* p3
cp ${INPUTS_DIR}/ref/* p4
cp ${INPUTS_DIR}/ref/* p5
cp ${INPUTS_DIR}/ref/* p6
cp ${INPUTS_DIR}/ref/* p7

perfCounters='INST_RETIRED,LONGEST_LAT_CACHE:REFERENCE,UNHALTED_CORE_CYCLES'

phaseLen=20000000
numPhases=2000
logFile="perfCtrs"
warmupPeriod=10
profilingPeriod=20

# processStr="$numPhases ${BUILD_DIR}/401.bzip2 input.source 64"
# processStr="$numPhases /bin/ls"

processStr1="$numPhases - 0  ${BUILD_DIR}/429.mcf inp.in"
processStr2="$numPhases - 1  ${BUILD_DIR}/429.mcf inp.in"
processStr3="$numPhases - 2  ${BUILD_DIR}/429.mcf inp.in"
processStr4="$numPhases - 3  ${BUILD_DIR}/429.mcf inp.in"
processStr5="$numPhases - 4  ${BUILD_DIR}/429.mcf inp.in"
processStr6="$numPhases - 5  ${BUILD_DIR}/429.mcf inp.in"
processStr7="$numPhases - 6  ${BUILD_DIR}/429.mcf inp.in"
processStr8="$numPhases - 7  ${BUILD_DIR}/429.mcf inp.in"

echo "$perfCounters $phaseLen $logFile $warmupPeriod $profilingPeriod -- $processStr1 -- $processStr2 -- $processStr3 -- $processStr4 -- $processStr5 -- $processStr6 -- $processStr7 -- $processStr8"

# Pin KPart thread to core 15 (hyperthreading enabled)
STARTTIME=$(($(date +%s%N)/1000000))
numactl -C 15 ./../src/kpart_cmt $perfCounters $phaseLen $logFile $warmupPeriod $profilingPeriod \
    -- $processStr1 \
    -- $processStr2 \
    -- $processStr3 \
    -- $processStr4 \
    -- $processStr5 \
    -- $processStr6 \
    -- $processStr7 \
    -- $processStr8

ENDTIME=$(($(date +%s%N)/1000000))
echo "Elapsed time = $(($ENDTIME - $STARTTIME)) milliseconds."
