CXX=g++
CC=gcc
FLAGS = -O3 -g -I./include
CXXFLAGS = $(FLAGS) -std=c++0x
CCFLAGS = $(FLAGS) 

BUILDDIR = build

TGTS = build/cat_cos build/cat_cbm 

INCLUDES = ./include/cpuid.h ./include/msr.h \
		   ./include/sysconfig.h ./include/msr_haswell.h \
		   ./include/cat.h ./include/cmt.h

all : $(BUILDDIR) $(TGTS)

$(BUILDDIR):
	mkdir -p $@

build/cat_cos : cat/cat_cos.cpp $(INCLUDES)
	$(CXX) $(CXXFLAGS) $< -o $@

build/cat_cbm : cat/cat_cbm.cpp $(INCLUDES)
	$(CXX) $(CXXFLAGS) $< -o $@

clean:
	rm -rf build
