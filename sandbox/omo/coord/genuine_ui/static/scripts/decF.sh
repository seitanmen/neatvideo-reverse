#!/bin/sh
# usage: decF.sh addr...  (decompile with the fully auto-analysed scratch project NVF; writes decomp_full/)
S=/Users/seitan/Desktop/work/reverse/sandbox/omo/coord/genuine_ui/static
P=/private/tmp/claude-501/-Users-seitan-Desktop-work-reverse/040b381d-b723-4608-b0cb-2e82227f9c2d/scratchpad/ghfull
~/tools/ghidra_12.1.4_PUBLIC/support/analyzeHeadless $P NVF -process NeatVideo6.arm64.ofx -noanalysis -readOnly -scriptPath $S/scripts -postScript DecompCreate.java $S/decomp_full "$@" > $P/dec.log 2>&1
grep -E "nofunc|ERROR|Exception" $P/dec.log | head
