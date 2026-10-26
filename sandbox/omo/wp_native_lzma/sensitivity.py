# Sensitivity of native/verify_lzma.py: mutate a copy of native/nv_lzma.cpp (in tmp/mut/), build it against the
# already built SDK objects (make -C native -f Makefile.lzma first), and run `verify_lzma.py --quick` with
# NV_LZMA_EXE pointing at the mutant.  Every mutant must be caught (exit 1, mismatches > 0).
import os
import subprocess

N = "/Users/seitan/Desktop/work/reverse/native"
M = os.path.join(os.path.dirname(os.path.abspath(__file__)), "tmp", "mut")
os.makedirs(M, exist_ok=True)
src = open(f"{N}/nv_lzma.cpp").read()
muts = {
    "destlen7f": ("size_t dl = n + 0x80;", "size_t dl = n + 0x7f;"),
    "destlen81": ("size_t dl = n + 0x80;", "size_t dl = n + 0x81;"),
    "dictnoround": ("uint32_t d = static_cast<uint32_t>(m & 0xfffff000u);",
                    "uint32_t d = static_cast<uint32_t>(n ? n : 1);"),
    "dict+4k": ("uint32_t d = static_cast<uint32_t>(m & 0xfffff000u);",
                "uint32_t d = static_cast<uint32_t>(m & 0xfffff000u) + 0x1000;"),
    "fb64": ("dict_size_for(n), 3, 0, 2, 32, 1)", "dict_size_for(n), 3, 0, 2, 64, 1)"),
    "lc2": ("dict_size_for(n), 3, 0, 2, 32, 1)", "dict_size_for(n), 2, 0, 2, 32, 1)"),
    "pb0": ("dict_size_for(n), 3, 0, 2, 32, 1)", "dict_size_for(n), 3, 0, 0, 32, 1)"),
    "level4": ("props, &ps, 5, dict_size_for", "props, &ps, 4, dict_size_for"),
    "level6": ("props, &ps, 5, dict_size_for", "props, &ps, 6, dict_size_for"),
    "trunc": ("else r = std::rint(x);", "else r = std::trunc(x);"),
    "round_away": ("else r = std::rint(x);", "else r = std::round(x);"),
    "nan32767": ("if (std::isnan(x)) r = 0.0f;", "if (std::isnan(x)) r = 32767.0f;"),
    "inf_swap": ("r = x > 0.0f ? 32767.0f : -32768.0f;", "r = x > 0.0f ? -32768.0f : 32767.0f;"),
    "clamp_lo": ("if (r < -32768.0f) r = -32768.0f;", "if (r < -32767.0f) r = -32767.0f;"),
    "clamp_hi": ("if (r > 32767.0f) r = 32767.0f;", "if (r > 32766.0f) r = 32766.0f;"),
    "scale_double": ("const float x = plane[i] * 16.0f;",
                     "const float x = static_cast<float>(static_cast<double>(plane[i]) * 16.0 + 1e-9);"),
    "hdrgarbage": ("buf.resize(16 + dl);", "buf[13] = 1; buf.resize(16 + dl);"),
    "endian": ("out[2 * i] = static_cast<uint8_t>(u & 0xffu);\n        out[2 * i + 1] = static_cast<uint8_t>(u >> 8);",
               "out[2 * i + 1] = static_cast<uint8_t>(u & 0xffu);\n        out[2 * i] = static_cast<uint8_t>(u >> 8);"),
}
objs = sorted(os.path.join(N, "lzma", "obj", o) for o in os.listdir(os.path.join(N, "lzma", "obj")))
caught = 0
for k, (a, b) in muts.items():
    assert src.count(a) == 1, k
    fn = os.path.join(M, f"nv_lzma_{k}.cpp")
    open(fn, "w").write(src.replace(a, b))
    exe = os.path.join(M, f"t_{k}")
    subprocess.run(["c++", "-std=c++17", "-O2", "-ffp-contract=off", "-fno-fast-math", "-D_7ZIP_ST", f"-I{N}",
                    "-o", exe, f"{N}/nv_lzma_main.cpp", fn] + objs, check=True)
    r = subprocess.run(["python3", f"{N}/verify_lzma.py", "--quick"], env=dict(os.environ, NV_LZMA_EXE=exe),
                       capture_output=True, text=True)
    tot = [ln for ln in r.stdout.splitlines() if ln.startswith("TOTAL")]
    caught += r.returncode == 1
    print(f"{k:14s} exit {r.returncode}  {tot[0] if tot else r.stderr[-200:]}", flush=True)
print(f"caught {caught}/{len(muts)}")
