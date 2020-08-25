all:libsatacc.so
include depend.mk
CXX=clang++
SRCS=$(wildcard $(PWD)/*.cc) 
OBJS=$(patsubst %.cc,%.o,$(SRCS))
CXXFLAGS=-I$(HOME)/usr/include -I../include -g -I$(HOME)/git/DRAMsim3/ext/headers -I$(HOME)/git/DRAMsim3/src  -std=c++20 -Wall -fPIC -O3 -DNDEBUG
LDFLAGS= -L $(HOME)/git/DRAMsim3/ -ldramsim3 -L$(HOME)/usr/lib -lsjqcache -shared
libsatacc.so:$(OBJS) depend.mk
	$(CXX) $(LDFLAGS) -o $@ $(OBJS)
.PHONY:clean
depend.mk: $(SRCS) 
	echo Making dependencies
	$(CXX) $(CXXFLAGS) $(SRCS)  -MM > depend.mk
clean:
	rm -rf *.o *.so
