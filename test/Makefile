CXXFLAGS=-I../include -g -I/home/sjq/git/DRAMsim3/ext/headers -I/home/sjq/git/DRAMsim3/src  -std=c++17 -Wall
LDFLAGS=-L /home/sjq/git/DRAMsim3/ -ldramsim3 -lsjqcache
CXX=g++
CC=gcc
all:main.out

main.out:main_test.o ../src/watcher.o ../src/clause.o watcher_test.o \
		clause_test.o watcher_clause_test.o dram_sim_test.o ../src/cache_interface.o \
		cache_interface_test.o ../src/acc.o lambda_test.o and_test.o
	$(CXX) $^ $(LDFLAGS) -o $@

.PHONY:clean
clean:
	rm -rf *.o ../src/*.o main.out