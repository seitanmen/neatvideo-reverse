#!/bin/sh
# usage: dec.sh addr...   (writes decomp/fn_0x*.c; read-only on the binary, project in scratchpad)
S=/Users/seitan/Desktop/work/reverse/sandbox/omo/coord/genuine_ui/static
P=/private/tmp/claude-501/-Users-seitan-Desktop-work-reverse/040b381d-b723-4608-b0cb-2e82227f9c2d/scratchpad/ghui
~/tools/ghidra_12.1.4_PUBLIC/support/analyzeHeadless $P NVUI -process NeatVideo6.arm64.ofx -noanalysis -scriptPath $S/scripts -postScript DecompCreate.java $S/decomp "$@" > $P/dec.log 2>&1
grep -E "nofunc|ERROR|Exception" $P/dec.log | head
