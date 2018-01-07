#!/bin/bash
APP_DIR=../lltools/build/traverse_list_lbm
INPUT=12288

for pid in 0 1 2 3 4 5 6 7
do
mkdir -p "p$pid"
done

# hardware counters to sample periodicially
# will be used for constructing KPart's online app cache profiles
perfCounters='INST_RETIRED,LONGEST_LAT_CACHE:REFERENCE,UNHALTED_CORE_CYCLES'

# KPart parameters:
phaseLen=20000000  #sample HW counters every 20M instr.
numPhases=1000 #measure the first 2000 phases in each running program
logFile="perfCtrs" #prefix for log file where hardware counters are stored
warmupPeriod=10  #start profiling after this init period (unit: B cycles)
profilingPeriod=10 #profile co-running applications every this much B cycles

# Examples
# processStr1="$numPhases ${BUILD_DIR}/401.bzip2 input.source 64"
# processStr1="$numPhases /bin/ls"

processStr1="$numPhases - 0 ${APP_DIR} ${INPUT}"
processStr2="$numPhases - 1 $APP_DIR $INPUT"
processStr3="$numPhases - 2 $APP_DIR $INPUT"
processStr4="$numPhases - 3 $APP_DIR $INPUT"
processStr5="$numPhases - 4 $APP_DIR $INPUT"
processStr6="$numPhases - 5 $APP_DIR $INPUT"
processStr7="$numPhases - 6 $APP_DIR $INPUT"
processStr8="$numPhases - 7 $APP_DIR $INPUT"

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
