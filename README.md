## KPart
KPart is a tool for partitioning the last-level cache (LLC) dynamically between co-running applications on an Intel multicore processor with <i>way-partitioning</i> support. KPart is designed to sidestep a key limitation of way-partitioning: coarse-grained partition sizes. 

KPart profiles the cache needs of co-running applications periodically, then groups the applications based on their cache profiles into clusters and maps cache partitions to these clusters. Applications within a cluster share their assigned partition, while applications across clusters remain isolated. More details on how profiling, clustering, and cache assignments are done are available in our [HPCA'18 paper](http://people.csail.mit.edu/sanchez/papers/2018.kpart.hpca.pdf).

### Platform
KPart was developed and tested on an 8-core Intel Broadwell D-1540 processor which supports LLC way-partitioning. The system was running Ubuntu 14.04, Linux kernel version 4.2.3. The LLC size is 12MB and supports up to 12 partitions, each of size 1MB. 


### Prerequisites
KPart requires the following libraries:
* [perfmon2: Performance Monitoring on Linux](http://perfmon2.sourceforge.net/)
* [armadillo: C++ Linear Algebra Library](http://arma.sourceforge.net/)
* Make sure [Cache Allocation Technology (CAT) and Cache Monitoring Technology (CMT)](https://www.intel.com/content/www/us/en/communications/cache-monitoring-cache-allocation-technologies.html) are supported, otherwise the tools under kpart/lltools/cat will not be able to monitor or partition the shared cache. 

### Building KPart

* Step 1: Build the libraries needed to access Intel's CAT and CMT features (under /kpart/lltools):
```
kpart$ cd lltools 
kpart/lltools$ make 
```

* Step 2: Update the LIBPFMPATH path in the Makefile under /kpart/src/ to point to your local LIBPFMPATH directory, then build KPart: 
```
kpart$ cd src 
kpart/src$ make 
```

### Running KPart 
```
Usage: ./kpart_cmt <comma-sep-events> <phase_len> <logfile/- for stdout> <warmup_period_B> <profile_period_B>-- <max_phases_1> <input_redirect_1/'-' for stdin>  <comma-sep-core-list> prog1 -- ...
```

#### Test Example
A testing script is available under [kpart/tests/example.sh](tests/example.sh). 
The script assumes SPEC-CPU2006 benchmark is available and runs multiple copies of SPEC apps, then profiles their cache needs and partitions the last-level cache among them using KPart. 

### Intel Bug with LLC Classes of Service 10, 11 
Discuss bug here. 

### Contributors  
* Nosayba El-Sayed, CSAIL MIT/QCRI, HBKU
* Harshad Kasture, Oracle 
* Anurag Mukkara, CSAIL MIT 
* Daniel Sanchez, CSAIL MIT 
* Po-An Tsai, CSAIL MIT 

### Publication 
[El-Sayed, Mukkara, Tsai, Kasture, Ma, and Sanchez, KPart: A Hybrid Cache Partitioning-Sharing Technique for Commodity Multicores, HPCA-24, Austria, 2018.](http://people.csail.mit.edu/sanchez/papers/2018.kpart.hpca.pdf)

### License
This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details

