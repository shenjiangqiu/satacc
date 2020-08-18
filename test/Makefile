CXXFLAGS=-I../include -g -I$(HOME)/git/DRAMsim3/ext/headers -I$(HOME)/git/DRAMsim3/src -I$(HOME)/usr/include  -std=c++17 -Wall -fPIC
LDFLAGS=-L$(HOME)/usr/lib -L$(HOME)/usr/lib64  -lfmt -L $(HOME)/git/DRAMsim3/ -ldramsim3 -lsjqcache 
CXX=g++
CC=gcc

TESTSRCS      = $(wildcard $(PWD)/*.cc) 
CORESRCS=$(wildcard $(PWD)/../src/*.cc) 

all:main.out depend.mk
include depend.mk




main.out:main_test.o ../src/watcher.o ../src/clause.o watcher_test.o \
		clause_test.o watcher_clause_test.o dram_sim_test.o ../src/cache_interface.o \
		cache_interface_test.o ../src/acc.o lambda_test.o and_test.o acc_test.o\
		../src/private_cache.o
	$(CXX) $^ $(LDFLAGS) -o $@
depend.mk: $(TESTSRCS) $(CORESRCS)
	echo Making dependencies
	$(CXX) $(CXXFLAGS) $(CORESRCS) $(TESTSRCS) -MM > depend.mk

.PHONY:clean
clean:
	rm -rf *.o ../src/*.o main.out
