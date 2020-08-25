CXXFLAGS=-I../include -g -I/home/sjq/git/DRAMsim3/ext/headers -I/home/sjq/git/DRAMsim3/src  -std=c++17 -Wall -fPIC -O0
LDFLAGS=-lfmt -L /home/sjq/git/DRAMsim3/ -ldramsim3 -lsjqcache 
CXX=clang++
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