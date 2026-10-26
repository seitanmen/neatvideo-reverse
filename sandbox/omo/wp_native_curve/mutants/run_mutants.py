# Sensitivity check of native/verify_curve.py: build mutated copies of native/nv_curve.cpp and confirm that
# the verification reports mismatches for each (a check that passes everything would be worthless).
#   python3 sandbox/omo/wp_native_curve/mutants/run_mutants.py
import os, re, subprocess, sys
HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, "..", "..", "..", ".."))
SRC = os.path.join(ROOT, "native", "nv_curve.cpp")
MUT = [
    ("ratio_eps NEON part as a true division (no FRECPE/FRECPS)",
     "        vst1q_f32(out + i, FM(vld1q_f32(a + i), FM(est, step)));",
     "        (void)est; (void)step; for (int q = 0; q < 4; ++q) out[i + q] = a[i + q] / (b[i + q] + eps);"),
    ("IsClip bin = trunc instead of frintn", "fcvtzs(std::rint(p));", "fcvtzs(std::trunc(p));"),
    ("row box without the NEON prefix-sum groups", "    const bool prefix4 = W > 2 * r;", "    const bool prefix4 = false;"),
    ("luma without FMA", "        out[i] = fma32(B[i], c, fma32(G[i], b, ar));", "        out[i] = FA(FM(B[i], c), FA(FM(G[i], b), ar));"),
    ("ramp NEON body unfused", "vfmaq_f32(vi, vld1q_f32(x + i), vs)", "vaddq_f32(vi, vmulq_f32(vs, vld1q_f32(x + i)))"),
    ("line padding +0.0 instead of -0.0", "    xp[HW] = -0.0f;", "    xp[HW] = 0.0f;"),
    ("smooth with the first-operand NaN rule (numpy scalar rule ignored)",
     "        ys[i] = add2(m[i], add2(m[i - 1], m[i + 1]) * 0.5) * 0.5;",
     "        ys[i] = add1(m[i], add1(m[i - 1], m[i + 1]) * 0.5) * 0.5;"),
    ("est * step commuted (NaN sign of FRECPS)", "vld1q_f32(a + i), FM(est, step)", "vld1q_f32(a + i), FM(step, est)"),
    ("F_i = f + g commuted (NaN operand order)", "            vadd(fp, gp, Fp, n);", "            vadd(gp, fp, Fp, n);"),
    ("running box S + x commuted (NaN operand order)", "                const auto v = FA(LD(S + l, t), LD(x + l, t));",
     "                const auto v = FA(LD(x + l, t), LD(S + l, t));"),
    ("double sums in float", "sums[k.bins[q]] = add1(sums[k.bins[q]], static_cast<double>(k.noise[q]));",
     "sums[k.bins[q]] = static_cast<float>(add1(sums[k.bins[q]], static_cast<double>(k.noise[q])));"),
]
src = open(SRC).read()
out_rows = []
for name, old, new in MUT:
    assert src.count(old) == 1, name
    d = os.path.join(HERE, "build")
    os.makedirs(d, exist_ok=True)
    cpp = os.path.join(d, "nv_curve.cpp")
    open(cpp, "w").write(src.replace(old, new))
    exe = os.path.join(d, "nv_curve_test_mut")
    subprocess.run(["c++", "-std=c++17", "-O2", "-ffp-contract=off", "-fno-fast-math", "-w", "-I", os.path.join(ROOT, "native"),
                    "-o", exe, os.path.join(ROOT, "native", "nv_curve_main.cpp"), cpp], check=True)
    r = subprocess.run([sys.executable, "-B", os.path.join(ROOT, "native", "verify_curve.py"), "--fuzz", "200", "--clips", "1500",
                        "--interp", "50"], env=dict(os.environ, NV_CURVE_EXE=exe), capture_output=True, text=True)
    summary = [l for l in r.stdout.splitlines() if re.match(r"(curve|clip|interp|tile|total)", l) and "images" not in l]
    detected = r.returncode != 0
    out_rows.append((name, detected, summary))
    print(f"[{'DETECTED' if detected else 'missed  '}] {name}")
    for l in summary:
        print("    " + l)
