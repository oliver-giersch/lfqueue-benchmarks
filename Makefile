CXX := c++

SRC := ./src
INC := -I./include -I./lib/looqueue/include -I./lib/scqueue/include -I./lib/ymcq/include -I./src
LIB := -lpthread

CXXFLAGS := -std=c++20 -O2

all: bench_deque bench_throughput

bench_throughput: $(SRC)/bench_throughput.cpp $(SRC)/common.o
	$(CXX) $(CXXFLAGS) $(INC) $^ -o $@

bench_deque: $(SRC)/bench_deque.cpp $(SRC)/common.o
	$(CXX) $(CXXFLAGS) $(INC) $^ -o $@

$(SRC)/common.o: $(SRC)/common.cpp
	$(CXX) $(CXXFLAGS) $(INC) -c $< -o $@

clean:
	$(RM) bench_throughput bench_deque 2> /dev/null || true

.PHONY: all clean

