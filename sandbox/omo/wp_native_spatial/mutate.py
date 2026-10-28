# Sensitivity test of native/verify_spatial.py and native/verify_temporal.py (N1h / N1g): inject one
# change at a time into a copy of the native v5 sources, build it, and check that the verification
# (random + component cases, --no-real) reports mismatches.  Run from anywhere:
#   python3 sandbox/omo/wp_native_spatial/mutate.py [NAME ...]
import os
import re
import shutil
import subprocess
import sys

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", ".."))
NATIVE = os.path.join(ROOT, "native")
WORK = os.path.join(os.path.dirname(os.path.abspath(__file__)), "mut")
SRC = ["nv_ndimage.cpp", "nv_ndimage.h", "nv_spatial.cpp", "nv_spatial.h", "nv_temporal.cpp", "nv_temporal.h",
       "nv_v5_main.cpp"]
FLAGS = "-std=c++17 -O2 -ffp-contract=off -fno-fast-math -Wall -Wextra".split()

# (name, file, old, new, count) - every `old` must occur exactly `count` times
MUTANTS = [
    ("gauss_nofma", "nv_ndimage.cpp", "acc = FMAD(s, fw[-4 + k], acc);", "acc = AD(acc, MD(s, fw[-4 + k]));", 1),
    ("gauss_order", "nv_ndimage.cpp", "double s = AD(il[x + jj], il[x - jj]);", "double s = AD(il[x - jj], il[x + jj]);", 1),
    ("uniform_order", "nv_ndimage.cpp", "tmp = AD(tmp, SD(buf[ll - 1 + size], buf[ll - 1]));",
     "tmp = SD(AD(tmp, buf[ll - 1 + size]), buf[ll - 1]);", 1),
    ("uniform_mul", "nv_ndimage.cpp", "o[ll] = (float)DD(tmp, fs);", "o[ll] = (float)MD(tmp, 1.0 / fs);", 1),
    ("mapc_w1", "nv_ndimage.cpp", "wy1 = SD(1.0, wy0);", "wy1 = ry;", 1),
    ("median_sort", "nv_ndimage.cpp", "out.at(y, x) = (float)ni_select(buf, 0, 8, 4);",
     "std::sort(buf, buf + 9); out.at(y, x) = (float)buf[4];", 1),
    ("interp_nofma", "nv_ndimage.cpp", "double r = FMAD(slope, SD(x, xp[j]), fp[j]);",
     "double r = AD(MD(slope, SD(x, xp[j])), fp[j]);", 1),
    ("down2_col1", "nv_ndimage.cpp", "if (W2 == 1) {", "if (W2 == -1) {", 1),
    ("b3_assoc", "nv_ndimage.cpp", "r[x] = A(A(M(w0, A(t[0][x], t[4][x])), M(w1, A(t[1][x], t[3][x]))), M(w2, t[2][x]));",
     "r[x] = A(M(w0, A(t[0][x], t[4][x])), A(M(w1, A(t[1][x], t[3][x])), M(w2, t[2][x])));", 1),
    ("npmax_fmax", "nv_ndimage.h", "inline float np_max(float a, float b) { return (a >= b || isnanf_(a)) ? a : b; }",
     "inline float np_max(float a, float b) { return std::fmax(a, b); }", 1),
    ("exp_double", "nv_ndimage.cpp", "float np_expf(float x) { return g_expf(x); }",
     "float np_expf(float x) { return (float)std::exp((double)x); }", 1),
    ("pw_tree", "nv_temporal.cpp", "auto T = [](float x, float y) { return A(y, x); };",
     "auto T = [](float x, float y) { return A(x, y); };", 1),
    ("pw_fold", "nv_temporal.cpp", "float res = a[0];", "float res = A(NEG0, a[0]);", 1),
    ("search_flat", "nv_temporal.cpp", "if (nbx == 1) {", "if (nbx == -1) {", 1),
    ("argmin_last", "nv_temporal.cpp", "if (v[i] < best) { best = v[i]; k = i; }",
     "if (v[i] <= best) { best = v[i]; k = i; }", 1),
    ("dense_half", "nv_temporal.cpp", "f32((bs - 1) / 2.0)", "f32((double)((bs - 1) / 2))", 1),
    ("warp_noclip", "nv_temporal.cpp", "cx.at(i, j) = np_clip(A((float)j, FX.at(i, j)), ZERO, wm);",
     "cx.at(i, j) = np_max(A((float)j, FX.at(i, j)), ZERO);", 1),
    ("pyr_eps", "nv_temporal.cpp", "A(M(s1.d[i], rj), EPS3)", "A(M(s1.d[i], rj), EPS6)", 1),
    ("sf_assoc", "nv_temporal.cpp", "M(M(NVAR, wv), wv)", "M(NVAR, M(wv, wv))", 1),
    ("rv_order", "nv_temporal.cpp", "r.d[i] = A(M(t, t), D(M(aa, sf[c].d[i]), M(sw[c].d[i], sw[c].d[i])));",
     "r.d[i] = A(D(M(aa, sf[c].d[i]), M(sw[c].d[i], sw[c].d[i])), M(t, t));", 1),
    ("spatial_eps_order", "nv_spatial.cpp", "A(e.d[i], eps6)", "A(eps6, e.d[i])", 1),
    ("spatial_g_noclamp", "nv_spatial.cpp", "D(np_max(S(e.d[i], M(M(sp, s), s)), 0.0f), A(e.d[i], eps6))",
     "D(S(e.d[i], M(M(sp, s), s)), A(e.d[i], eps6))", 1),
]


def build(name, file, old, new, count):
    d = os.path.join(WORK, name)
    os.makedirs(d, exist_ok=True)
    for f in SRC:
        shutil.copy(os.path.join(NATIVE, f), d)
    p = os.path.join(d, file)
    s = open(p).read()
    assert s.count(old) == count, (name, s.count(old))
    s = s.replace(old, new)
    if "std::sort" in new or "std::fmax" in new or "std::exp" in new:
        s = "#include <algorithm>\n#include <cmath>\n" + s
    open(p, "w").write(s)
    exe = os.path.join(d, "nv_v5_test")
    srcs = [os.path.join(d, f) for f in SRC if f.endswith(".cpp")]
    subprocess.run(["c++", *FLAGS, "-o", exe, *srcs, "-lpthread"], check=True, capture_output=True)
    return exe


def run(script, exe):
    r = subprocess.run([sys.executable, os.path.join(NATIVE, script), "--no-real", "--exe", exe],
                       capture_output=True, text=True)
    m = re.search(r"^(\d+)/(\d+) bit-exact", r.stdout, re.M)
    ok, n = (int(m.group(1)), int(m.group(2))) if m else (-1, -1)
    return n - ok if m else None


def main():
    names = set(sys.argv[1:])
    rows = []
    for mt in MUTANTS:
        if names and mt[0] not in names:
            continue
        exe = build(*mt)
        bs, bt = run("verify_spatial.py", exe), run("verify_temporal.py", exe)
        det = (bs or 0) + (bt or 0) > 0
        rows.append((mt[0], bs, bt, det))
        print(f"{mt[0]:20s} spatial mismatching cases {bs}, temporal {bt} -> {'DETECTED' if det else 'not detected'}",
              flush=True)
    print(f"{sum(r[3] for r in rows)}/{len(rows)} detected")


if __name__ == "__main__":
    main()
