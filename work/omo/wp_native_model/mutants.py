# Sensitivity test of native/verify_model.py: inject one deliberate error at a time into a copy of
# native/nv_model.cpp, build it into tmp/, and run the verification against the cached Python results
# (python3 native/verify_model.py --exe ...).  A mutant is "detected" when the run reports mismatches.
#   python3 sandbox/omo/wp_native_model/mutants.py [--quick] [names...]
import os
import re
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, "..", "..", ".."))
NATIVE = os.path.join(ROOT, "native")
TMP = os.path.join(HERE, "tmp", "mut")

# name: (description, old, new) - old must occur exactly once in nv_model.cpp
MUTANTS = {
    "recip_div": ("FUN_009ef0d0 NEON part: true division instead of FRECPE + FRECPS",
                  "const float w = M(est, FRECPS(z2, est));", "const float w = 1.0f / z2;"),
    "ratio_div": ("FUN_009ee04c NEON part: true division instead of FRECPE + FRECPS",
                  "out[i] = M(a[i], M(est, step));", "out[i] = D(a[i], d); (void)step;"),
    "aggr_nofma": ("FUN_00ad15f0 aggregation: separate multiply and add instead of FMLA",
                   "drow[q] = FMA(wrow[q], srow[q], drow[q]);", "drow[q] = A(M(wrow[q], srow[q]), drow[q]);"),
    "std_nofma": ("FUN_009ef664 block std: separate multiply and add instead of FMLA",
                  "acc[l] = FMA(d, d, acc[l]);", "acc[l] = A(acc[l], M(d, d));"),
    "blend_nofma": ("FUN_009ed978 NEON part: non-fused like the tail",
                    "out[i] = A(Av[i], FMA(Bv[i], one, zero));", "out[i] = A(A(Av[i], M(Bv[i], one)), zero);"),
    "argmax_gt": ("FUN_009ee5fc: strict > (the earlier direction wins ties)",
                  "if (x[p] >= best[p]) {", "if (x[p] > best[p]) {"),
    "crop_1px": ("measure crop one pixel wider at the left",
                 "for (int x = bx; x < W - bx; ++x) o.push_back", "for (int x = (bx ? bx - 1 : 0); x < W - bx; ++x) o.push_back"),
    "level_c": ("DAT_012445a8[0] = 1.0 instead of 1.06",
                "const float LEVEL_C[5] = {bits(0x3f87ae14u),", "const float LEVEL_C[5] = {bits(0x3f800000u),"),
    "u8_floor": ("FUN_009f1110: floor instead of round-half-even", "r = std::nearbyint(r);", "r = std::floor(r);"),
    "sel_last": ("select_greedy: the last minimum instead of the first", "if (first || c < best) {",
                 "if (first || c <= best) {"),
    "pad_sym": ("pad_mirror: symmetric (edge repeated) instead of reflect",
                "return i < 0 ? -i : (i >= n ? 2 * (n - 1) - i : i);", "return i < 0 ? -i - 1 : (i >= n ? 2 * n - 1 - i : i);"),
    "lines_order": ("oriented running box: S = x + S instead of S + x (NaN payload only)",
                    "for (int l = 0; l < L; ++l) s[l] = A(s[l], a[l]);\n    }\n    for (int j = 0;",
                    "for (int l = 0; l < L; ++l) s[l] = A(a[l], s[l]);\n    }\n    for (int j = 0;"),
    "table_noflag": ("FUN_00acf420: no neighbour bins (flag 0)", "if (center <= a) {", "if (false && center <= a) {"),
    "extrap_fac": ("extrapolation factor fixed at 0.5", "const float fac = li.fac;", "const float fac = C05;"),
    "sad_bound": ("sad_grid: a block touching the padded border counts as outside",
                  "any = X + dx >= 0 && X + dx + bs <= Wp", "any = X + dx > 0 && X + dx + bs <= Wp"),
    "sad_bound2": ("sad_grid: a block touching the padded border counts as outside (per position)",
                   "if (!(X + dx >= 0 && X + dx + bs <= Wp", "if (!(X + dx > 0 && X + dx + bs <= Wp"),
    "border_div": ("FUN_00ad3c88 border columns: multiply by 1/cnt instead of dividing",
                   "if (cnt > 0) acc = D(acc, static_cast<float>(cnt));",
                   "if (cnt > 0) acc = M(acc, 1.0f / static_cast<float>(cnt));"),
    "box3_last": ("box3_fused last row: (h + h_prev) + 0 instead of 0 + (h + h_prev) (zero sign / NaN only)",
                  "out.at(H - 1, x) = A(0.0f, A(h.at(H - 1, x), h.at(H - 2, x)));",
                  "out.at(H - 1, x) = A(A(h.at(H - 1, x), h.at(H - 2, x)), 0.0f);"),
    "robust_tail": ("FUN_009ef0d0 scalar tail: unordered compare counts as > 1", "|| isnanf_(z);", "&& !isnanf_(z);"),
    "up2_odd": ("up2: odd column copied from column 2Wc - 2", "row[2 * Wc] = row[2 * Wc - 1];",
                "row[2 * Wc] = row[2 * Wc - 2];"),
    "score_thr": ("FUN_00ad319c: tiled threshold 0.1 instead of 0.15", "thr = C_THR_TILED;", "thr = C_THR;"),
}


def main():
    names = [a for a in sys.argv[1:] if not a.startswith("--")] or list(MUTANTS)
    quick = "--quick" in sys.argv
    os.makedirs(TMP, exist_ok=True)
    src = open(os.path.join(NATIVE, "nv_model.cpp")).read()
    log = open(os.path.join(HERE, "mutants.log"), "a")
    log.write(f"# {time.strftime('%Y-%m-%d %H:%M:%S')}\n")
    for name in names:
        desc, old, new = MUTANTS[name]
        if src.count(old) != 1:
            print(f"{name}: pattern found {src.count(old)} times, skipped")
            continue
        mdir = os.path.join(TMP, name)
        os.makedirs(mdir, exist_ok=True)
        cpp = os.path.join(mdir, "nv_model.cpp")
        open(cpp, "w").write(src.replace(old, new))
        exe = os.path.join(mdir, "nv_model_test")
        b = subprocess.run(["clang++", "-std=c++17", "-O2", "-ffp-contract=off", "-fno-fast-math", "-w",
                            f"-I{NATIVE}", "-o", exe, os.path.join(NATIVE, "nv_model_main.cpp"), cpp,
                            os.path.join(NATIVE, "nv_band.cpp")], capture_output=True, text=True)
        if b.returncode:
            print(f"{name}: build failed: {b.stderr[-400:]}")
            continue
        args = [sys.executable, os.path.join(NATIVE, "verify_model.py"), "--exe", exe]
        if quick:
            args.append("--quick")
        r = subprocess.run(args, capture_output=True, text=True)
        summ = [l for l in r.stdout.splitlines() if "bit-exact (" in l]
        line = summ[-1] if summ else r.stdout[-300:]
        m = re.search(r"\((\d+) MISMATCHES\)", line)
        verdict = f"DETECTED ({m.group(1)} runs differ)" if m else ("not detected" if r.returncode == 0 else
                                                                 f"exit {r.returncode}")
        msg = f"{name:13s} {verdict:28s} {desc}"
        print(msg, flush=True)
        log.write(msg + "\n")
        log.flush()


if __name__ == "__main__":
    main()
