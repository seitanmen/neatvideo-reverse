# G9: replays the libnvcore calls recorded by native/x86_bridge_cap.py on another build of the library and compares
# every buffer after the call (and the return value, and the number of cancel queries) with the recording, byte
# for byte.  Standard library only (ctypes, pickle), so an x86_64 Python without numpy runs it under Rosetta 2:
#
#   arch -x86_64 /Library/Frameworks/Python.framework/Versions/3.12/bin/python3.12 native/x86_bridge_replay.py \
#       --cap DIR --lib native/libnvcore_x86.dylib --out result.json [--dump DIR] [--only fn,..] [--every N]
#
# Each call gets fresh buffers holding the recorded before-call bytes (the same address in one call = the same
# buffer, as recorded), pointer arrays rebuilt on them, the recorded scalars and strings, and a cancel callback
# that gives the recorded answers in order (query k > recorded: the last answer).  --dump DIR writes the
# after-call bytes of the buffers that differ (<pid>_<seq>_r<k>.bin), so native/x86_bridge_diff.py can count the
# differing words against an arm64 replay of the same calls.  --save-hashes FILE keeps the after-call hashes of
# this replay; --ref-hashes FILE compares with such a file instead of the recording (two replays, e.g. the
# -DNV_LIBM_ALT builds of both architectures).  A replay of no call fails (exit 2), and so does --require-groups
# g1,.. when a group of verify_bridge.py (group_of) has no recorded call other than nvc_abi_version (an empty or
# missing capture; fallback, which makes no other native call, counts its nvc_abi_version calls).
# Exit codes: 0 all identical; 3 calls differ (or unspecified calls); 2 no capture / no call / a group missing;
# 1 (Python's own) an exception - only 3 means a finished comparison with differences.
# Each buffer difference names its argument ("arg": index in the C call, "entry": index in a pointer array).
import argparse
import ctypes
import glob
import hashlib
import json
import os
import pickle
import platform
import struct
import sys
import time

TYPES = {"c_void_p": ctypes.c_void_p, "c_int": ctypes.c_int, "c_long": ctypes.c_long, "c_size_t": ctypes.c_size_t,
         "c_ulong": ctypes.c_ulong, "c_longlong": ctypes.c_longlong, "c_ulonglong": ctypes.c_ulonglong,
         "c_double": ctypes.c_double, "c_float": ctypes.c_float, "c_char_p": ctypes.c_char_p,
         "CFUNC_int": ctypes.CFUNCTYPE(ctypes.c_int)}
CANCEL = TYPES["CFUNC_int"]
# the functions nvcore_c*.h declares void: their "return value" under ctypes' default restype (c_int) is whatever
# the return register holds, so it is not compared
VOID = {"nvc_flat_area", "nvc_aft_combine", "nvc_yuv_to_rgb", "nvc_minmax", "nvc_render_simple", "nvc_freq_images",
        "nvc_render_freq", "nvc_not_uniform", "nvc_mix", "nvc_temporal_mask", "nvc_nlc_image", "nvc_interp"}


# verify_bridge.py group of a recorded call, from the (module, function) label of x86_bridge_cap._label
_VB = {"case_curve": "curve", "case_profile": "profile", "case_aft": "aft", "case_frame": "frame",
       "case_engine": "frame", "case_threads": "frame", "worker": "frame", "case_cancel": "cancel",
       "case_fallback": "fallback", "_child_native": "fallback"}


def group_of(label):
    mod, fn = label
    if mod == "verify_bridge":
        return _VB.get(fn, "verify_bridge:" + fn)
    if mod.startswith("verify_bridge_depth"):
        return "depth"
    if mod.startswith("verify_bridge_ui"):
        return "ui"
    if "prof5" in mod:
        return "prof5"
    if mod == "export":             # nv_ui_qt.export and its denoise_video_v5 workers (verify_bridge.py --export)
        return "export"
    if mod == "?":
        return "unlabelled"
    return f"{mod}:{fn}"


def records(cap):
    for p in sorted(glob.glob(os.path.join(cap, "cap_*.pkl"))):
        with open(p, "rb") as f:
            while True:
                try:
                    yield pickle.load(f)
                except EOFError:
                    break


class Blobs:
    def __init__(self, cap):
        self.cap, self.last = cap, {}

    def get(self, b):
        if b[0] == "i":
            return b[1]
        h = b[1]
        if h not in self.last:
            if len(self.last) > 64:
                self.last.clear()
            with open(os.path.join(self.cap, "cas", h), "rb") as f:
                self.last[h] = f.read()
        return self.last[h]


def replay(lib, rec, blobs):
    """(buffers after the call, return value, cancel query counts, recorded answer counts)."""
    fn = getattr(lib, rec["fn"])
    if rec["argtypes"] is not None:
        fn.argtypes = [TYPES[t] for t in rec["argtypes"]]
    fn.restype = TYPES[rec["restype"]] if rec["restype"] is not None else None
    bufs = [ctypes.create_string_buffer(blobs.get(r["before"]), r["n"]) for r in rec["regions"]]
    keep, calls, qn, rn = [], [], [], []

    def ptr(e):
        if e[0] == "null":
            return None
        if e[0] == "z":
            z = ctypes.create_string_buffer(16)
            keep.append(z)
            return ctypes.addressof(z)
        return ctypes.addressof(bufs[e[1]])
    for e in rec["args"]:
        k = e[0]
        if k == "v":
            calls.append(e[1])
        elif k == "s":
            calls.append(e[1])
        elif k in ("null", "z", "r"):
            calls.append(ptr(e))
        elif k == "a":
            arr = (ctypes.c_void_p * max(len(e[1]), 1))(*[ptr(x) for x in e[1]])
            keep.append(arr)
            calls.append(ctypes.addressof(arr))
        elif k == "cb":
            if e[1] is None:
                calls.append(CANCEL())
            else:
                ans, cnt = list(e[1]), [0]

                def f(ans=ans, cnt=cnt):
                    i = cnt[0]
                    cnt[0] += 1
                    return ans[i] if i < len(ans) else (ans[-1] if ans else 0)
                cb = CANCEL(f)
                keep.append(cb)
                calls.append(cb)
                qn.append(cnt)
                rn.append(len(ans))
        else:
            raise ValueError(f"unknown argument kind {k}")
    ret = fn(*calls)
    if rec["fn"] in VOID:
        ret = None
    return [b.raw for b in bufs], ret, [c[0] for c in qn], rn


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--cap", required=True)
    ap.add_argument("--lib", required=True)
    ap.add_argument("--out", required=True)
    ap.add_argument("--dump")
    ap.add_argument("--only")
    ap.add_argument("--every", type=int, default=1)
    ap.add_argument("--select", help="pid:seq,... (only these calls; with --dump-all: dump every buffer)")
    ap.add_argument("--dump-all", action="store_true")
    ap.add_argument("--save-hashes")
    ap.add_argument("--ref-hashes")
    ap.add_argument("--require-groups", help="g1,..: fail when one of these groups has no recorded call")
    A = ap.parse_args()
    ref = None
    if A.ref_hashes:
        with open(A.ref_hashes) as f:
            ref = json.load(f)
    saved = {}
    lib = ctypes.CDLL(os.path.abspath(A.lib))
    blobs = Blobs(A.cap)
    only = set(A.only.split(",")) if A.only else None
    sel = set(A.select.split(",")) if A.select else None
    if A.dump:
        os.makedirs(A.dump, exist_ok=True)
    groups, bad, unspec, overlap = {}, [], 0, 0
    t0, n_seen, seen_groups = time.time(), 0, {}
    if not glob.glob(os.path.join(A.cap, "cap_*.pkl")):
        print(f"{A.lib}: FAIL: no capture (no cap_*.pkl in {A.cap})")
        return 2
    for rec in records(A.cap):
        if rec.get("unspec"):
            unspec += 1
            continue
        if only and rec["fn"] not in only:
            continue
        n_seen += 1
        gk = group_of(rec["label"])
        # nvc_abi_version (the library check at load time) does not count, except in fallback, whose processes
        # by design make no other native call
        if rec["fn"] != "nvc_abi_version" or gk == "fallback":
            seen_groups[gk] = seen_groups.get(gk, 0) + 1
        if (n_seen - 1) % A.every:
            continue
        key = f"{rec['pid']}:{rec['seq']}"
        if sel is not None and key not in sel:
            continue
        overlap += bool(rec.get("overlap"))
        after, ret, qn, rn = replay(lib, rec, blobs)
        g = groups.setdefault(f"{rec['label'][0]}:{rec['label'][1]}|{rec['fn']}",
                              {"calls": 0, "ok": 0, "out_words": 0, "out_bytes": 0, "all_bytes": 0, "t": 0.0})
        g["calls"] += 1
        g["t"] += rec["t"]
        diffs = []
        where = {}                      # region -> (argument, pointer-array entry or None)
        for i, e in enumerate(rec["args"]):
            if e[0] == "r":
                where.setdefault(e[1], (i, None))
            elif e[0] == "a":
                for j, x in enumerate(e[1]):
                    if x[0] == "r":
                        where.setdefault(x[1], (i, j))
        hs = [hashlib.sha256(a).hexdigest() for a in after]
        saved[key] = {"h": hs, "ret": struct.pack("<d", ret).hex() if isinstance(ret, float) else ret, "q": qn}
        if ref is not None:
            if key not in ref:
                bad.append({"call": key, "fn": rec["fn"], "label": rec["label"], "diffs": ["not in --ref-hashes"]})
                continue
            rr_ = ref[key]
            for k, (r, h) in enumerate(zip(rec["regions"], hs)):
                g["all_bytes"] += r["n"]
                if r["role"] != "in" or r["changed"]:
                    g["out_bytes"] += r["n"]
                    g["out_words"] += r["n"] // r["esize"]
                if h != rr_["h"][k]:
                    diffs.append({"region": k, "role": r["role"], "n": r["n"], "esize": r["esize"],
                                  "arg": where.get(k, (None, None))[0], "entry": where.get(k, (None, None))[1]})
                    if A.dump:
                        with open(os.path.join(A.dump, f"{rec['pid']}_{rec['seq']}_r{k}.bin"), "wb") as f:
                            f.write(after[k])
            if saved[key]["ret"] != rr_["ret"]:
                diffs.append({"ret": [saved[key]["ret"], rr_["ret"]]})
            if qn != rr_["q"]:
                diffs.append({"cancel_queries": [qn, rr_["q"]]})
            if diffs:
                bad.append({"call": key, "fn": rec["fn"], "label": rec["label"], "diffs": diffs})
            else:
                g["ok"] += 1
            continue
        for k, (r, a) in enumerate(zip(rec["regions"], after)):
            g["all_bytes"] += r["n"]
            if r["role"] != "in" or r["changed"]:
                g["out_bytes"] += r["n"]
                g["out_words"] += r["n"] // r["esize"]
            same = hs[k] == r["after_sha"]
            if not same:
                d = {"region": k, "role": r["role"], "n": r["n"], "esize": r["esize"],
                     "arg": where.get(k, (None, None))[0], "entry": where.get(k, (None, None))[1]}
                if r["after"] is not None:
                    w = r["esize"]
                    d["words_differ"] = sum(a[i:i + w] != r["after"][i:i + w] for i in range(0, r["n"], w))
                diffs.append(d)
            if A.dump and (not same or A.dump_all):
                with open(os.path.join(A.dump, f"{rec['pid']}_{rec['seq']}_r{k}.bin"), "wb") as f:
                    f.write(a)
        rr = rec["ret"] if rec["fn"] not in VOID else None
        if isinstance(ret, float) or isinstance(rr, float):     # floats by their bits (NaN payloads too)
            ret_same = isinstance(ret, float) and isinstance(rr, float) and \
                struct.pack("<d", ret) == struct.pack("<d", rr)
        else:
            ret_same = ret == rr
        if not ret_same:
            diffs.append({"ret": [repr(ret), repr(rr)]})
        if qn != rn:
            diffs.append({"cancel_queries": [qn, rn]})
        if diffs:
            bad.append({"call": key, "fn": rec["fn"], "label": rec["label"], "diffs": diffs})
        else:
            g["ok"] += 1
    res = {"lib": A.lib, "machine": platform.machine(), "python": sys.version.split()[0], "every": A.every,
           "unspec": unspec, "overlap": overlap, "groups": groups, "bad": bad, "seconds": time.time() - t0}
    with open(A.out, "w") as f:
        json.dump(res, f, indent=1)
    if A.save_hashes:
        with open(A.save_hashes, "w") as f:
            json.dump(saved, f)
    calls = sum(g["calls"] for g in groups.values())
    ok = sum(g["ok"] for g in groups.values())
    words = sum(g["out_words"] for g in groups.values())
    print(f"{A.lib} on {platform.machine()}: {ok}/{calls} calls identical, {words} output words, "
          f"{len(bad)} differ, unspecified {unspec}, overlapping buffers {overlap} ({res['seconds']:.0f} s)")
    for b in bad[:20]:
        print("  DIFF", b["call"], b["fn"], b["label"], b["diffs"][:4])
    if calls == 0:
        print(f"{A.lib}: FAIL: no call replayed")
        return 2
    if A.require_groups:
        miss = [g for g in A.require_groups.split(",") if g and not seen_groups.get(g)]
        print("  recorded calls by group: " + ", ".join(f"{g} {n}" for g, n in sorted(seen_groups.items())))
        if miss:
            print(f"{A.lib}: FAIL: no recorded call of the group(s) {','.join(miss)}")
            return 2
    return 3 if bad or unspec else 0


if __name__ == "__main__":
    sys.exit(main())
