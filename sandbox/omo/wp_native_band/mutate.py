# Sensitivity test of native/verify_band.py: each mutation changes one detail of native/nv_band.cpp
# (a copy under mut/), builds it and checks that verify_band.py reports mismatches.
#   python3 sandbox/omo/wp_native_band/mutate.py
import os, subprocess, sys
HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, "..", "..", ".."))
NAT = os.path.join(ROOT, "native")
SRC = open(os.path.join(NAT, "nv_band.cpp")).read()
MUT = [
    ("plain_add", 'inline float A(float a, float b) {\n#if defined(__aarch64__)', 'inline float A(float a, float b) {\n#if 0'),
    ("std_nofma", "acc[j] = std::fma(dd, dd, acc[j]);         // FMLA", "acc[j] = A(acc[j], dd * dd);"),
    ("std_tail_fma", "s = A(s, t * t);", "s = std::fma(t, t, s);"),
    ("reduce8_order", "return A(A(b0, b1), A(b2, b3));", "return A(A(b0, b2), A(b1, b3));"),
    ("block_len", "const size_t BLOCK = 0x800;", "const size_t BLOCK = 0x400;"),
    ("no_box3", "if (r == 1 && W >= 8 && W % 4 == 0) {", "if (false) {"),
    ("no_prefix4", "        prefix4 = true;\n", "        prefix4 = false;\n"),
    ("count_nan", "return n <= 4096 ? n : 0;", "return 0;"),
    ("no_interp49", "    if (n <= 49) {\n        if (n < 1) {", "    if (n <= 0) {\n        if (n < 1) {"),
    ("gq_index", "GQ_SMALL_BITS[d < 11 ? d : 11]", "GQ_SMALL_BITS[d < 10 ? d : 10]"),
    ("lim_min", "lim = lim > 10 ? lim : 10;", "lim = lim > 9 ? lim : 9;"),
    ("scalar_up2_thr", "if (Hc < 2 || Wc < 4) {", "if (Hc < 2 || Wc < 3) {"),
    ("fac_clamp", "fac = r < C0_5 ? C0_5 :", "fac = r < C0_25 ? C0_5 :"),
    ("cv_octaves", "        if (j < 7) {\n            nxt.resize", "        if (j < 6) {\n            nxt.resize"),
    ("down2_neon_cols", "const int c = (W & ~7) >> 1;", "const int c = (W & ~3) >> 1;"),
    ("cv_fma_add", "d[i] = std::fma(X[i], one, nm);", "d[i] = A(X[i], nm);"),
    ("border_shrink", "> 31 || (bx == 0 && by == 0)", "> 30 || (bx == 0 && by == 0)"),
    ("extrap_copycv", "for (int j = 0; j < 8; ++j) out[15 + 8 * k + j] = out[15 + 8 * (k - 1) + j];", ""),
    ("up2_odd_col", "o[c] = A(o[c - 1] - s[c - 1], s[c]);", "o[c] = A(s[c], o[c - 1] - s[c - 1]);"),
    ("scalar_order_t", "t = AS(step, t);", "t = A(step, t);"),
    ("scalar_order_g", "AS(MS(probit(q), sd) *", "AS(M(probit(q), sd) *"),
    ("probit_order", "return AS((C1 - f) *", "return A((C1 - f) *"),
    ("box_mode0_start", "S[l] = A(first[l] * fr1, S[l]);", "S[l] = A(S[l], first[l] * fr1);"),
]
os.makedirs(os.path.join(HERE, "mut"), exist_ok=True)
res = []
for name, a, b in MUT:
    assert SRC.count(a) >= 1, name
    cpp = os.path.join(HERE, "mut", f"{name}.cpp")
    exe = os.path.join(HERE, "mut", f"{name}.exe")
    open(cpp, "w").write(SRC.replace(a, b).replace('#include "nv_band.h"', f'#include "{NAT}/nv_band.h"'))
    subprocess.run(["c++", "-std=c++17", "-O2", "-ffp-contract=off", "-fno-fast-math", "-w", "-I", NAT, "-o", exe,
                    os.path.join(NAT, "nv_band_main.cpp"), cpp], check=True)
    r = subprocess.run([sys.executable, os.path.join(NAT, "verify_band.py"), "--cache", "--exe", exe],
                       capture_output=True, text=True)
    last = [l for l in r.stdout.splitlines() if "bit-exact" in l and "/" in l and l[0].isdigit()]
    res.append((name, r.returncode != 0, last[-1] if last else r.stdout[-300:] + r.stderr[-300:]))
    print(f"{name:16s} {'DETECTED' if r.returncode else 'missed  '}  {res[-1][2]}", flush=True)
print(f"{sum(d for _, d, _ in res)}/{len(res)} mutations detected")
