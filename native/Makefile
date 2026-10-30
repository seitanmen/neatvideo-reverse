# Native ports verified bit-exact against the Python implementations.
# No fast-math, no FMA contraction: the ports must reproduce the Python (and genuine)
# float32 op order exactly.
#
#   make -C native && python3 native/verify_quality.py   (N0)
#   make -C native && python3 native/verify_band.py      (N1a)
#   make -C native && python3 native/verify_curve.py     (N1b)
#   make -C native && python3 native/verify_lzma.py      (N1c)
#   make -C native && python3 native/verify_flat.py      (N1d)
#   make -C native && python3 native/verify_flat_cancel.py  (N1d status 5, F1)
#   make -C native && python3 native/verify_model.py     (N1e)
#   make -C native && python3 native/verify_temporal.py  (N1g)
#   make -C native && python3 native/verify_spatial.py   (N1h)
#   make -C native && python3 native/verify_profile.py   (N1f)
#   make -C native && python3 native/verify_aft.py       (N1i)
#   make -C native verify                                 (all of the above)
#   make -C native portable x86 && native/verify_x86.sh  (F4: portable build, x86_64 vs arm64 bytes)
#   make -C native lib && python3 native/verify_bridge.py   (N2a: libnvcore.dylib via ctypes vs the Python path)
#   native/verify_x86_bridge.sh   (G9: libnvcore_x86.dylib (`make lib_x86`) vs libnvcore.dylib on the recorded
#                                  verify_bridge.py calls, byte for byte)
#   make -C native libm-alt && python3 native/verify_libm_alt.py   (W2a: openlibm / musl / naive libm builds,
#                                                                   Makefile.libm_alt; the default build is unchanged)
#
# Makefile.band / .curve / .lzma / .flat / .model / .v5 / .profile / .aft are the per-task makefiles the workers wrote;
# the rules below are the same.

CXX ?= clang++
CC = clang
CXXFLAGS ?= -std=c++17 -O2 -ffp-contract=off -fno-fast-math -Wall -Wextra
# LZMA SDK 9.20 C sources (lzma/SOURCE.md): single threaded as the genuine's call; the SDK's own
# five benign warnings are silenced by name, the sources are not edited.
SDK_CFLAGS = -O2 -ffp-contract=off -fno-fast-math -D_7ZIP_ST -Wall -Wextra \
             -Wno-self-assign -Wno-unused-but-set-variable -Wno-misleading-indentation
SDK_SRC = lzma/LzmaLib.c lzma/LzmaEnc.c lzma/LzmaDec.c lzma/LzFind.c lzma/Alloc.c
# SDK_OBJDIR / SDK_XFLAGS: another architecture's objects in their own directory (`make lib_x86`)
SDK_OBJDIR ?= lzma/obj
SDK_XFLAGS ?=
SDK_OBJ = $(SDK_SRC:lzma/%.c=$(SDK_OBJDIR)/%.o)
SDK_HDR = $(wildcard lzma/*.h)

all: nv_quality_test nv_band_test nv_curve_test nv_lzma_test nv_flat_test nv_model_test nv_v5_test lib \
     nv_profile_test nv_aft_test

nv_quality_test: main.cpp nv_quality.cpp nv_quality.h
	$(CXX) $(CXXFLAGS) -o $@ main.cpp nv_quality.cpp

# nv_portable: the float operations with the AArch64 result on every architecture (N1a / N1b / N1d / N1e);
# nv_linekernel: the line kernels of FUN_00990448 (repro/nv_lines.py), shared by N1b and N1e.
# The four binaries below also build as <name>_p (-DNV_PORTABLE: the portable operations on arm64,
# `make portable`) and <name>_x86 (x86_64, run by Rosetta 2, `make x86`; native/verify_x86.sh).
PT = nv_portable.cpp nv_portable.h
LK = nv_linekernel.cpp nv_linekernel.h
SUF ?=
XFLAGS ?=
PORT_BINS = nv_band_test nv_curve_test nv_flat_test nv_model_test

# N1a: 5-level band measure (repro/nv_band.py)
nv_band_test$(SUF): nv_band_main.cpp nv_band.cpp nv_band.h $(PT)
	$(CXX) $(CXXFLAGS) $(XFLAGS) -o $@ nv_band_main.cpp nv_band.cpp nv_portable.cpp

# N1b: EstimateNoiseLevelCurve (repro/nv_curve.py + nv_pixnoise.py + nv_lines.py)
nv_curve_test$(SUF): nv_curve_main.cpp nv_curve.cpp nv_curve.h $(LK) $(PT)
	$(CXX) $(CXXFLAGS) $(XFLAGS) -o $@ nv_curve_main.cpp nv_curve.cpp nv_linekernel.cpp nv_portable.cpp -lpthread

# N1c: LZMA (repro/nv_lzma.py) over the SDK 9.20 sources
$(SDK_OBJDIR)/%.o: lzma/%.c $(SDK_HDR)
	@mkdir -p $(SDK_OBJDIR)
	$(CC) $(SDK_CFLAGS) $(SDK_XFLAGS) -c -o $@ $<

nv_lzma_test: nv_lzma_main.cpp nv_lzma.cpp nv_lzma.h $(SDK_OBJ)
	$(CXX) $(CXXFLAGS) -D_7ZIP_ST -o $@ nv_lzma_main.cpp nv_lzma.cpp $(SDK_OBJ)

# N1d: FindFlatArea (repro/nv_flat.py) over N1a + N1b
nv_flat_test$(SUF): nv_flat_main.cpp nv_flat.cpp nv_flat.h nv_band.cpp nv_band.h nv_curve.cpp nv_curve.h $(LK) $(PT)
	$(CXX) $(CXXFLAGS) $(XFLAGS) -o $@ nv_flat_main.cpp nv_flat.cpp nv_band.cpp nv_curve.cpp nv_linekernel.cpp nv_portable.cpp -lpthread

# N1e: BuildNoiseModel per-plane model, all 4096 words (repro/nv_model.py + _special + _bm) over N1a
nv_model_test$(SUF): nv_model_main.cpp nv_model.cpp nv_model.h nv_band.cpp nv_band.h $(LK) $(PT)
	$(CXX) $(CXXFLAGS) $(XFLAGS) -o $@ nv_model_main.cpp nv_model.cpp nv_band.cpp nv_linekernel.cpp nv_portable.cpp

# N1g / N1h: v5 temporal and spatial kernels (repro/nvfilter.py temporal_v5 / spatial_v5 /
# process_frame_v5); nv_ndimage reproduces the scipy 1.15.3 / numpy 2.1.3 primitives they use
V5_SRC = nv_v5_main.cpp nv_ndimage.cpp nv_spatial.cpp nv_temporal.cpp
V5_HDR = nv_ndimage.h nv_spatial.h nv_temporal.h

nv_v5_test: $(V5_SRC) $(V5_HDR)
	$(CXX) $(CXXFLAGS) -o $@ $(V5_SRC) -lpthread

# N1f: BuildNoiseModel profile, the 0xc300-byte object (repro/nv_profile.py + nv_mcstats + nv_refnoise)
# over N1a-N1e
PROF_SRC = nv_profile_main.cpp nv_profile.cpp nv_model.cpp nv_flat.cpp nv_band.cpp nv_curve.cpp \
           nv_linekernel.cpp nv_portable.cpp nv_lzma.cpp
PROF_HDR = nv_profile.h nv_model.h nv_flat.h nv_band.h nv_curve.h nv_linekernel.h nv_portable.h nv_lzma.h

nv_profile_test: $(PROF_SRC) $(PROF_HDR) $(SDK_OBJ)
	$(CXX) $(CXXFLAGS) -D_7ZIP_ST -o $@ $(PROF_SRC) $(SDK_OBJ) -lpthread

# N1i: AutoFineTune (aft_port/src/nv_aft.py) + the nv_aft_v5.apply_aft combination over N1a/N1b/N1e/N1g
AFT_SRC = nv_aft_main.cpp nv_aft.cpp nv_band.cpp nv_curve.cpp nv_model.cpp nv_ndimage.cpp \
          nv_linekernel.cpp nv_portable.cpp
AFT_HDR = nv_aft.h nv_band.h nv_curve.h nv_model.h nv_ndimage.h nv_linekernel.h nv_portable.h

nv_aft_test: $(AFT_SRC) $(AFT_HDR)
	$(CXX) $(CXXFLAGS) $(MUTFLAGS) -o $@ $(AFT_SRC) -lpthread

# N2a: libnvcore.dylib, the C ABI (nvcore_c.h) of the N1 ports for ctypes (repro/nv_native.py,
# native/verify_bridge.py).  `make lib` builds it; `make lib SUF=_p XFLAGS=-DNV_PORTABLE` (= `make lib_portable`)
# builds libnvcore_p.dylib with the portable float operations.
LIB_SRC = nvcore_c.cpp nv_aft.cpp nv_profile.cpp nv_model.cpp nv_flat.cpp nv_band.cpp nv_curve.cpp \
          nv_linekernel.cpp nv_portable.cpp nv_lzma.cpp nv_ndimage.cpp nv_spatial.cpp nv_temporal.cpp \
          nv_ui.cpp nvcore_c_ui.cpp nv_quality.cpp nv_prof5.cpp nv_lstsq.cpp nvcore_c_prof5.cpp \
          nv_depth.cpp nvcore_c_depth.cpp nvcore_c_rgb.cpp
LIB_HDR = nvcore_c.h nv_aft.h nv_profile.h nv_model.h nv_flat.h nv_band.h nv_curve.h nv_linekernel.h \
          nv_portable.h nv_lzma.h nv_ndimage.h nv_spatial.h nv_temporal.h \
          nv_ui.h nvcore_c_ui.h nv_quality.h nv_prof5.h nv_lstsq.h nvcore_c_prof5.h \
          nv_depth.h nvcore_c_depth.h nvcore_c_rgb.h

libnvcore$(SUF).dylib: $(LIB_SRC) $(LIB_HDR) $(SDK_OBJ)
	$(CXX) $(CXXFLAGS) $(XFLAGS) -D_7ZIP_ST -dynamiclib -install_name @rpath/libnvcore$(SUF).dylib \
	    -o $@ $(LIB_SRC) $(SDK_OBJ) -lpthread

lib: libnvcore$(SUF).dylib

lib_portable:
	$(MAKE) SUF=_p XFLAGS=-DNV_PORTABLE libnvcore_p.dylib

# G9: libnvcore_x86.dylib (x86_64, run by an x86_64 Python under Rosetta 2; native/verify_x86_bridge.sh)
lib_x86:
	$(MAKE) SUF=_x86 XFLAGS="-arch x86_64" SDK_XFLAGS="-arch x86_64" SDK_OBJDIR=lzma/obj_x86 libnvcore_x86.dylib

portable:
	$(MAKE) SUF=_p XFLAGS=-DNV_PORTABLE $(PORT_BINS:%=%_p)

x86:
	$(MAKE) SUF=_x86 XFLAGS="-arch x86_64" $(PORT_BINS:%=%_x86)

# JOBS=N passes --jobs N to the scripts that take it (default: each script's own default)
VJ = $(if $(JOBS),--jobs $(JOBS))

verify: all
	python3 verify_quality.py
	python3 verify_band.py $(VJ)
	python3 verify_curve.py $(VJ)
	python3 verify_lzma.py $(VJ)
	python3 verify_flat.py $(VJ)
	python3 verify_flat_cancel.py $(VJ)
	python3 verify_model.py $(VJ)
	python3 verify_temporal.py $(VJ)
	python3 verify_spatial.py $(VJ)
	python3 verify_profile.py $(VJ)
	python3 verify_aft.py $(VJ)

# N2a: the ctypes bridge against the Python path (needs the test videos, ffmpeg and PySide6: nv_ui_qt.state)
verify_bridge: lib
	python3 verify_bridge.py $(VJ)

clean:
	rm -rf nv_quality_test tmp_blobs.bin tmp_results.bin nv_band_test nv_curve_test \
	       nv_lzma_test lzma/obj lzma/obj_x86 nv_flat_test nv_model_test nv_v5_test nv_profile_test nv_aft_test \
	       $(PORT_BINS:%=%_p) $(PORT_BINS:%=%_x86) libnvcore.dylib libnvcore_p.dylib libnvcore_x86.dylib

# W2a: the libm sensitivity builds in libm_alt/bin/ (Makefile.libm_alt, libm_alt.h)
libm-alt:
	$(MAKE) -f Makefile.libm_alt

libm-alt-clean:
	$(MAKE) -f Makefile.libm_alt clean

.PHONY: all lib lib_portable lib_x86 portable x86 verify verify_bridge clean libm-alt libm-alt-clean
