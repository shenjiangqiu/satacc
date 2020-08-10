CPPFLAGS=-I../include -g -I/home/sjq/git/DRAMsim3/ext/headers -I/home/sjq/git/DRAMsim3/src 
LDFLAGS=-L /home/sjq/git/DRAMsim3/ -ldramsim3
main.out:main_test.o ../src/watcher.o ../src/clause.o watcher_test.o clause_test.o watcher_clause_test.o dram_sim_test.o
	g++ $^ $(LDFLAGS) -o $@