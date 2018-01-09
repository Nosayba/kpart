#!/bin/bash

# NOTE: Setting the governor to performance is enough. Even when switching back to userspace, turbo is on and setspeed is set to max.
# Get the highest frequency, which enables turbo.
freq=`cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_available_frequencies | cut -f1 -d" "`
echo "Will set max frequency at $freq"
find /sys/devices/system/cpu/ -name scaling_setspeed | xargs -n1 -IX bash -c "echo $freq > X"

# Set userspace governor
find /sys/devices/system/cpu/ -name scaling_governor | xargs -n1 -IX bash -c "echo ondemand > X"

