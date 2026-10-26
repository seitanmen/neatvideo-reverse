# N0: native ports verified bit-exact against the Python implementations.
# No fast-math, no FMA contraction: the ports must reproduce the Python (and genuine)
# float32 op order exactly.

CXX ?= clang++
CXXFLAGS ?= -std=c++17 -O2 -ffp-contract=off -fno-fast-math -Wall -Wextra

all: nv_quality_test

nv_quality_test: main.cpp nv_quality.cpp nv_quality.h
	$(CXX) $(CXXFLAGS) -o $@ main.cpp nv_quality.cpp

verify: nv_quality_test
	python3 verify_quality.py

clean:
	rm -f nv_quality_test tmp_blobs.bin tmp_results.bin

.PHONY: all verify clean
