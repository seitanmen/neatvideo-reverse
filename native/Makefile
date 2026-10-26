# Native ports verified bit-exact against the Python implementations.
# No fast-math, no FMA contraction: the ports must reproduce the Python (and genuine)
# float32 op order exactly.
#
#   make -C native && python3 native/verify_quality.py   (N0)
#   make -C native && python3 native/verify_band.py      (N1a)
#   make -C native && python3 native/verify_curve.py     (N1b)
#   make -C native && python3 native/verify_lzma.py      (N1c)
#   make -C native verify                                 (all of the above)
#
# Makefile.band / Makefile.curve / Makefile.lzma are the per-task makefiles the workers wrote;
# the rules below are the same.

CXX ?= clang++
CC = clang
CXXFLAGS ?= -std=c++17 -O2 -ffp-contract=off -fno-fast-math -Wall -Wextra
# LZMA SDK 9.20 C sources (lzma/SOURCE.md): single threaded as the genuine's call; the SDK's own
# five benign warnings are silenced by name, the sources are not edited.
SDK_CFLAGS = -O2 -ffp-contract=off -fno-fast-math -D_7ZIP_ST -Wall -Wextra \
             -Wno-self-assign -Wno-unused-but-set-variable -Wno-misleading-indentation
SDK_SRC = lzma/LzmaLib.c lzma/LzmaEnc.c lzma/LzmaDec.c lzma/LzFind.c lzma/Alloc.c
SDK_OBJ = $(SDK_SRC:lzma/%.c=lzma/obj/%.o)
SDK_HDR = $(wildcard lzma/*.h)

all: nv_quality_test nv_band_test nv_curve_test nv_lzma_test

nv_quality_test: main.cpp nv_quality.cpp nv_quality.h
	$(CXX) $(CXXFLAGS) -o $@ main.cpp nv_quality.cpp

# N1a: 5-level band measure (repro/nv_band.py)
nv_band_test: nv_band_main.cpp nv_band.cpp nv_band.h
	$(CXX) $(CXXFLAGS) -o $@ nv_band_main.cpp nv_band.cpp

# N1b: EstimateNoiseLevelCurve (repro/nv_curve.py + nv_pixnoise.py + nv_lines.py)
nv_curve_test: nv_curve_main.cpp nv_curve.cpp nv_curve.h
	$(CXX) $(CXXFLAGS) -o $@ nv_curve_main.cpp nv_curve.cpp -lpthread

# N1c: LZMA (repro/nv_lzma.py) over the SDK 9.20 sources
lzma/obj/%.o: lzma/%.c $(SDK_HDR)
	@mkdir -p lzma/obj
	$(CC) $(SDK_CFLAGS) -c -o $@ $<

nv_lzma_test: nv_lzma_main.cpp nv_lzma.cpp nv_lzma.h $(SDK_OBJ)
	$(CXX) $(CXXFLAGS) -D_7ZIP_ST -o $@ nv_lzma_main.cpp nv_lzma.cpp $(SDK_OBJ)

verify: all
	python3 verify_quality.py
	python3 verify_band.py
	python3 verify_curve.py
	python3 verify_lzma.py

clean:
	rm -rf nv_quality_test tmp_blobs.bin tmp_results.bin nv_band_test nv_curve_test tmp_curve_*.bin \
	       nv_lzma_test lzma/obj

.PHONY: all verify clean
