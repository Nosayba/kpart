#!/usr/bin/python

import os

path='/sys/devices/system/cpu/cpu0/cpuidle'
print '%10s | %10s' % ('State', 'Latency (us)')
for d in os.listdir(path):
    if not 'latency' in os.listdir('%s/%s' % (path, d)): continue
    
    fd = open('%s/%s/latency' % (path, d), 'r')
    latency = fd.readline().strip()
    fd.close()

    fd = open('%s/%s/name' % (path, d), 'r')
    name = fd.readline().strip()
    fd.close()

    print '%10s | %10s' % (name, latency)
