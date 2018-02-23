# Update LIBPFMPATH variable by making it point to location 
# of libpfm4 library on your machine 
LIBPFMPATH ?= ../libpfm-4.9.0

LLTOOLSPATH = ../lltools

CXX=g++
CC=gcc
# Flags for git libpfm
LDFLAGS += -Wl,-R$(LIBPFMPATH)/lib -L$(LIBPFMPATH)/lib -lpfm -pthread
CFLAGS += -g -O3 -I$(LIBPFMPATH)/include -I$(LIBPFMPATH)/perf_examples \
		  -DCONFIG_PFMLIB_DEBUG -DCONFIG_PFMLIB_OS_LINUX -I. -D_GNU_SOURCE
CXXFLAGS = $(CFLAGS) -std=c++0x
CXXFLAGS_CMT = -DUSE_CMT -I$(LLTOOLSPATH)/include
CXXFLAGS_MASTER = -DMASTER_PROC
PU_SRC = $(LIBPFMPATH)/perf_examples/perf_util.c
CLUST_SRC=$(wildcard cluster/*.cpp)

default: kpart

kpart : kpart.o perf_util.o cache_utils.cpp $(CLUST_SRC)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

kpart_master : kpart_master.o perf_util.o cache_utils.cpp $(CLUST_SRC)
	$(CXX) -o $@ $^ $(LDFLAGS)

kpart.o : kpart.cpp 
	$(CXX) $(CXXFLAGS) $(CXXFLAGS_CMT) -o $@ -c $<

kpart_master.o : kpart.cpp
	$(CXX) $(CXXFLAGS) $(CXXFLAGS_CMT) $(CXXFLAGS_MASTER) -o $@ -c $<

perf_util.o : $(PU_SRC)
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	rm -f *.o kpart
