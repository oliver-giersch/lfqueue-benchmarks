CXX := c++

SRC := ./src
INC := -I./include -I./lib/looqueue/include -I./lib/scqueue/include -I./lib/ymcqueue/include -I./src
LIB := -lpthread

CXXFLAGS := -std=c++20 -O2

bench_deque: $(SRC)/bench_deque.cpp
	$(CXX) $(CXXFLAGS) $(INC) -c $< -o $@

