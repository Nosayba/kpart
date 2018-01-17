#!/bin/bash
BUILD_DIR=/path/to/spec/benchmarks/build/speccpu2006/429.mcf
INPUTS_DIR=/path/to/spec/benchmarks/inputs/speccpu2006/429.mcf

for pid in 0 1 2 3 4 5 6 7
do
mkdir -p "p$pid"
cp ${INPUTS_DIR}/ref/* "p$pid"
done

# hardware counters to sample periodicially
# will be used for constructing KPart's online app cache profiles
perfCounters='INST_RETIRED,LONGEST_LAT_CACHE:REFERENCE,UNHALTED_CORE_CYCLES'

# KPart parameters:
phaseLen=20000000  #sample HW counters every 20M instr.
numPhases=2000 #measure the first 2000 phases in each running program
logFile="perfCtrs" #prefix for log file where hardware counters are stored
warmupPeriod=10  #start profiling after this init period (unit: B cycles)
profilingPeriod=20 #profile co-running applications every this much B cycles

# Examples
# processStr1="$numPhases ${BUILD_DIR}/401.bzip2 input.source 64"
# processStr1="$numPhases /bin/ls"

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
