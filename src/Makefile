SRCS=$(wildcard $(PWD)/*.cc) 
OBJS=$(patsubst %.cc,%.o,$(SRCS))
CXXFLAGS=-I$(HOME)/usr/include -I../include -g -I$(HOME)/git/DRAMsim3/ext/headers -I$(HOME)/git/DRAMsim3/src  -std=c++17 -Wall -fPIC -O3 -DNDEBUG
LDFLAGS=-L$(HOME)/usr/lib64 -L$(HOME)/usr/lib -L $(HOME)/git/DRAMsim3/ -ldramsim3 -L$(HOME)/usr/lib -lsjqcache -shared
libsatacc.so:$(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $^
.PHONY:clean
clean:
	rm -rf *.o *.so
