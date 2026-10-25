#!/bin/sh
# usage: run.sh addr...   (decompile into notes/dec using the project copy in tmp/)
W=/Users/seitan/Desktop/work/reverse/sandbox/omo/wp_motion
~/tools/ghidra_12.1.4_PUBLIC/support/analyzeHeadless $W/tmp/ghproj NV -process NeatVideo6.arm64.ofx -noanalysis -readOnly -scriptPath $W/src/gh -postScript DecompAddrs.java $W/notes/dec "$@" 2>&1 | tail -5
