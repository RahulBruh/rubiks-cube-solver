CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -Wpedantic
EMCC     ?= emcc

CORE  = src/cubie.cpp src/coords.cpp src/solver.cpp
HDRS  = src/cubie.hpp src/coords.hpp src/solver.hpp

.PHONY: all test bench wasm clean

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

# WebAssembly build for the browser UI: writes web/solver.js + web/solver.wasm.
wasm: web/solver.js

web/solver.js: $(CORE) src/wasm_api.cpp $(HDRS)
	$(EMCC) -std=c++17 -O3 $(CORE) src/wasm_api.cpp -o $@ \
	  -sMODULARIZE=1 -sEXPORT_NAME=createSolverModule \
	  -sENVIRONMENT=web,worker,node -sALLOW_MEMORY_GROWTH=1 \
	  -sEXPORTED_FUNCTIONS=_cs_init,_cs_solve,_cs_random_state \
	  -sEXPORTED_RUNTIME_METHODS=cwrap

clean:
	rm -rf build web/solver.js web/solver.wasm
