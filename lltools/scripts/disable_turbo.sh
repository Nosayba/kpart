#!/bin/bash

# Set userspace governor
find /sys/devices/system/cpu/ -name scaling_governor | xargs -n1 -IX bash -c "echo userspace > X"

# Get the second highest frequency... which disables turbo
freq=`cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_available_frequencies | cut -f2 -d" "`
echo "Will fix frequency at $freq"
find /sys/devices/system/cpu/ -name scaling_setspeed | xargs -n1 -IX bash -c "echo $freq > X"
