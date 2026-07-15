CXX := c++

SRC := ./src
INC := -I./include -I./lib/looqueue/include -I./lib/scqueue/include -I./lib/ymcq/include -I./src
LIB := -lpthread

CXXFLAGS := -std=c++20 -O2

all: bench_deque

bench_deque: $(SRC)/bench_deque.cpp
	$(CXX) $(CXXFLAGS) $(INC) $< -o $@

clean:
	$(RM) bench_deque 2> /dev/null || true

.PHONY: all clean

