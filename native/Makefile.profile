# N1f: BuildNoiseModel profile port (nv_profile, the 0xc300-byte object of repro/nv_profile.py) over
# nv_band (N1a), nv_curve (N1b), nv_lzma + lzma/ (N1c), nv_flat (N1d) and nv_model (N1e) - separate
# makefile until the lead merges it into native/Makefile.
#   make -C native -f Makefile.profile && python3 native/verify_profile.py
# Same flags as native/Makefile: no fast-math, no FMA contraction.

CXX ?= clang++
CC = clang
CXXFLAGS ?= -std=c++17 -O2 -ffp-contract=off -fno-fast-math -Wall -Wextra
SDK_CFLAGS = -O2 -ffp-contract=off -fno-fast-math -D_7ZIP_ST -Wall -Wextra \
             -Wno-self-assign -Wno-unused-but-set-variable -Wno-misleading-indentation
SDK_SRC = lzma/LzmaLib.c lzma/LzmaEnc.c lzma/LzmaDec.c lzma/LzFind.c lzma/Alloc.c
SDK_OBJ = $(SDK_SRC:lzma/%.c=lzma/obj/%.o)
SDK_HDR = $(wildcard lzma/*.h)

PROF_SRC = nv_profile_main.cpp nv_profile.cpp nv_model.cpp nv_flat.cpp nv_band.cpp nv_curve.cpp nv_linekernel.cpp nv_portable.cpp nv_lzma.cpp
PROF_HDR = nv_profile.h nv_model.h nv_flat.h nv_band.h nv_curve.h nv_linekernel.h nv_portable.h nv_lzma.h

all: nv_profile_test

lzma/obj/%.o: lzma/%.c $(SDK_HDR)
	@mkdir -p lzma/obj
	$(CC) $(SDK_CFLAGS) -c -o $@ $<

nv_profile_test: $(PROF_SRC) $(PROF_HDR) $(SDK_OBJ)
	$(CXX) $(CXXFLAGS) -D_7ZIP_ST -o $@ $(PROF_SRC) $(SDK_OBJ) -lpthread

verify: nv_profile_test
	python3 verify_profile.py

clean:
	rm -f nv_profile_test

.PHONY: all verify clean
