CXX=g++
CC=gcc
FLAGS = -O3 -g -I./include
CXXFLAGS = $(FLAGS) -std=c++0x
CCFLAGS = $(FLAGS) 

BUILDDIR = build

TGTS = build/cat_cos build/cat_cbm build/cat_config build/cmt_config \
	   build/cmt_rmid build/cpudmalatency build/ilp build/dvfslat \
	   build/setFreq build/getFreq \
	   build/availableFreqs build/powerSpecs build/prefetch \
	   build/procid build/lmb_example build/traverse_list_lbm

INCLUDES = ./include/cpuid.h ./include/msr.h \
		   ./include/sysconfig.h ./include/timetools.h \
		   ./include/dvfs.h ./include/msr_haswell.h \
		   ./include/energy_meter.h ./include/cat.h \
		   ./include/cmt.h

all : $(BUILDDIR) $(TGTS)

$(BUILDDIR):
	mkdir -p $@

build/cat_cos : cat/cat_cos.cpp $(INCLUDES)
	$(CXX) $(CXXFLAGS) $< -o $@

build/cat_cbm : cat/cat_cbm.cpp $(INCLUDES)
	$(CXX) $(CXXFLAGS) $< -o $@

build/cat_config : cat/cat_config.cpp $(INCLUDES)
	$(CXX) $(CXXFLAGS) $< -o $@

build/cmt_config : cat/cmt_config.cpp $(INCLUDES)
	$(CXX) $(CXXFLAGS) $< -o $@

build/cmt_rmid : cat/cmt_rmid.cpp $(INCLUDES)
	$(CXX) $(CXXFLAGS) $< -o $@

build/cpudmalatency : power/cpudmalatency.c
	$(CC) $(CCFLAGS) power/cpudmalatency.c -o $@

build/ilp : power/ilp.cpp $(INCLUDES)
	$(CXX) $(CXXFLAGS) $< -o $@ -lrt

build/dvfslat : power/dvfslat.cpp $(INCLUDES)
	$(CXX) $(CXXFLAGS) $< -o $@ -lrt

build/setFreq : power/setFreq.cpp $(INCLUDES)
	$(CXX) $(CXXFLAGS) $< -o $@ -lrt

build/getFreq : power/getFreq.cpp $(INCLUDES)
	$(CXX) $(CXXFLAGS) $< -o $@ -lrt

build/availableFreqs : power/availableFreqs.cpp $(INCLUDES)
	$(CXX) $(CXXFLAGS) $< -o $@ 

build/powerSpecs : power/powerSpecs.cpp $(INCLUDES)
	$(CXX) $(CXXFLAGS) $< -o $@

build/prefetch : misc/prefetch.cpp $(INCLUDES)
	$(CXX) $(CXXFLAGS) $< -o $@

build/procid : misc/procid.cpp $(INCLUDES)
	$(CXX) $(CXXFLAGS) $< -o $@

build/lmb_example : cat/lmb_example.cpp $(INCLUDES)
	$(CXX) $(CXXFLAGS) $< -o $@

build/traverse_list_lbm : cat/traverse_list_lbm.cpp $(INCLUDES)
	$(CXX) $(CXXFLAGS) $< -o $@

clean:
	rm build/*
