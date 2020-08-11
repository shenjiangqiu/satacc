CXXFLAGS=-I../include -g -I/home/sjq/git/DRAMsim3/ext/headers -I/home/sjq/git/DRAMsim3/src  -std=c++17 -Wall -fPIC
LDFLAGS=-lfmt -L /home/sjq/git/DRAMsim3/ -ldramsim3 -lsjqcache 
CXX=g++
CC=gcc

TESTSRCS      = $(wildcard $(PWD)/*.cc) 
CORESRCS=$(wildcard $(PWD)/../src/*.cc) 

all:main.out depend.mk
include depend.mk




main.out:main_test.o ../src/watcher.o ../src/clause.o watcher_test.o \
		clause_test.o watcher_clause_test.o dram_sim_test.o ../src/cache_interface.o \
		cache_interface_test.o ../src/acc.o lambda_test.o and_test.o acc_test.o
	$(CXX) $^ $(LDFLAGS) -o $@
depend.mk: $(TESTSRCS) $(CORESRCS)
	echo Making dependencies
	$(CXX) $(CXXFLAGS) $(CORESRCS) $(TESTSRCS) -MM > depend.mk

.PHONY:clean
clean:
	rm -rf *.o ../src/*.o main.out