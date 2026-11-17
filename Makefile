CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -Wpedantic

CORE  = src/cubie.cpp src/coords.cpp src/solver.cpp
HDRS  = src/cubie.hpp src/coords.hpp src/solver.hpp

.PHONY: all test bench clean

all: build/cubesolver build/test_solver

build/cubesolver: $(CORE) src/main.cpp $(HDRS)
	@mkdir -p build
	$(CXX) $(CXXFLAGS) -o $@ $(CORE) src/main.cpp

build/test_solver: $(CORE) tests/test_solver.cpp $(HDRS)
	@mkdir -p build
	$(CXX) $(CXXFLAGS) -o $@ $(CORE) tests/test_solver.cpp

test: build/test_solver
	./build/test_solver

bench: build/cubesolver
	./build/cubesolver bench 500

clean:
	rm -rf build
