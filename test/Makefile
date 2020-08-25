CXXFLAGS=-I../include -g -I$(HOME)/git/DRAMsim3/ext/headers -I$(HOME)/git/DRAMsim3/src -I$(HOME)/usr/include  -std=c++17 -Wall -fPIC -O3
LDFLAGS=-L$(HOME)/usr/lib -L$(HOME)/usr/lib64  -lfmt -L $(HOME)/git/DRAMsim3/ -ldramsim3 -lsjqcache 
CXX=g++
CC=gcc

TESTSRCS      = $(wildcard $(PWD)/*.cc) 
OBJS=$(patsubst %.cc,%.o,$(TESTSRCS))

CORESRCS=$(wildcard $(PWD)/../src/*.cc) 

all:main.out depend.mk
include depend.mk




main.out:$(OBJS) ../src/libsatacc.so
	$(CXX) $(OBJS) -L../src -lsatacc $(LDFLAGS) -o $@
depend.mk: $(TESTSRCS) $(CORESRCS)
	echo Making dependencies
	$(CXX) $(CXXFLAGS) $(CORESRCS) $(TESTSRCS) -MM > depend.mk

.PHONY:clean
clean:
	rm -rf *.o ../src/*.o main.out
