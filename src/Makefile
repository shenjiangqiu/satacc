SRCS=$(wildcard $(PWD)/*.cc) 
OBJS=$(patsubst %.cc,%.o,$(SRCS))
CXXFLAGS=-I../include -g -I/home/sjq/git/DRAMsim3/ext/headers -I/home/sjq/git/DRAMsim3/src  -std=c++17 -Wall -fPIC
LDFLAGS=-lfmt -L /home/sjq/git/DRAMsim3/ -ldramsim3 -lsjqcache -shared
libsatacc.so:$(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $^
