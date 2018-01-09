#!/usr/bin/python

from optparse import OptionParser
import os
import shlex
import subprocess

################################################################################
# Helper functions
################################################################################
def availableGovernors():
    fd = open(('/sys/devices/system/cpu/cpu0/cpufreq/'
               'scaling_available_governors'), 'r')
    line = fd.read().strip()
    fd.close()

    available = line.split(' ')
    return available

def curGovernors():
    ret = {}
    for (root, dirs, files) in os.walk('/sys/devices/system/cpu'):
        if not 'scaling_governor' in files: continue
        cpu = root.split('/')[-2]
        fd = open('%s/%s' % (root, 'scaling_governor'), 'r')
        governor = fd.readlines()[0].strip()
        fd.close()
        ret[cpu] = governor
    return ret

################################################################################
# Command line ops
################################################################################
parser = OptionParser()
parser.add_option('-s', '--set', dest='governor', action='store', \
                  type='string', help='Set scaling governor', default=None)
parser.add_option('-g', '--get', dest='get', action='store_true', \
                  help='Report current scaling governor', default=False)
parser.add_option('-a', '--available', dest='available', action='store_true', \
                  help='List available governors', default=False)
(options, args) = parser.parse_args()

if options.available: 
    print 'available governors : ' + ' '.join(availableGovernors())
elif options.get:
    governors = curGovernors()
    for cpu in sorted(governors.keys()):
        print '%s : %s' % (cpu, governors[cpu])
elif options.governor is None or \
     not options.governor in availableGovernors():
    parser.print_help()
else:
    for (root, dirs, files) in os.walk('/sys/devices/system/cpu'):
        for f in files:
            if f == 'scaling_governor':
                fd = open('%s/%s' % (root, f), 'w')
                fd.write('%s\n' % options.governor)
                fd.close()


