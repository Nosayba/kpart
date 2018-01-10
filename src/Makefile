# Update LIBPFMPATH variable by making it point to location 
# of libpfm4 library on your machine 
LIBPFMPATH ?= /path/to/perfmon2-libpfm4
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

default: kpart kpart_cmt kpart_cmt_master

kpart: kpart.o perf_util.o
	$(CXX) -o $@ $^ $(LDFLAGS)

kpart_cmt : kpart_cmt.o perf_util.o
	$(CXX) -o $@ $^ $(LDFLAGS)

kpart_cmt_master : kpart_cmt_master.o perf_util.o
	$(CXX) -o $@ $^ $(LDFLAGS)

kpart.o : kpart.cpp
	$(CXX) $(CXXFLAGS) -o $@ -c $<

kpart_cmt.o : kpart.cpp
	$(CXX) $(CXXFLAGS) $(CXXFLAGS_CMT) -o $@ -c $<

kpart_cmt_master.o : kpart.cpp
	$(CXX) $(CXXFLAGS) $(CXXFLAGS_CMT) $(CXXFLAGS_MASTER) -o $@ -c $<

perf_util.o : $(PU_SRC)
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	rm -f *.o kpart
