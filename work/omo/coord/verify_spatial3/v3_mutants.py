"""感度試験: 写しの関数をわざと変えて v3_leaves の比較が不一致を出すか。"""
import sys, numpy as np
sys.dont_write_bytecode = True
sys.argv = ["x"]
sys.path.insert(0, "/Users/seitan/Desktop/work/reverse/sandbox/omo/coord/verify_spatial3")
import v3_leaves as V
sp = V.sp; F = np.float32
orig = {k: getattr(sp, k) for k in ("leaf_rstep", "leaf_rinv", "leaf_recip", "leaf_blend", "strip_box3", "strip_rms3", "leaf_sel2", "ixform_neon", "sigfuse_port", "leaf_u8", "leaf_acc")}
def mut_rstep_nan(s, x0, x1, neon=True):  # 端数部の NaN を x0 にしない (旧式)
    x0_, x1_ = sp._f32(x0).reshape(-1), sp._f32(x1).reshape(-1); si = sp._div(F(1.0), F(s)); n4 = sp._split(x0_.size, neon)
    out = orig["leaf_rstep"](s, x0, x1, neon)
    if n4 < x0_.size:
        v = sp._mul(si, sp._sub(x1_[n4:], x0_[n4:])); out = out.copy()
        out[n4:] = np.where(sp._fabs(v) <= 1.0, x0_[n4:], sp._div(x0_[n4:], sp._mul(v, v))).astype(F)
    return out
def mut_rinv_exactdiv(s, x0, x1, neon=True):  # NEON 部も真の除算
    return orig["leaf_rinv"](s, x0, x1, False)
def mut_recip_div(eps, x, neon=True): return orig["leaf_recip"](eps, x, False)
def mut_blend_swap(a, b, x, y, neon=True): return orig["leaf_blend"](b, a, x, y, neon)
def mut_box_mode(x, mode=0): return orig["strip_box3"](x, 1 - mode)
def mut_rms_s(x, mode=1, s=1.0): return orig["strip_rms3"](x, mode, 1.0)
def mut_sel2_flag(a, b, i3, i4, best, src, wt, flag, stride, neon=True): return orig["leaf_sel2"](a, b, i3, i4, best, src, wt, 0, stride, neon)
def mut_ix(Y, a, b):
    o = orig["ixform_neon"](Y, a, b); return [o[1], o[0], o[2]]
def mut_u8(x, neon=True):
    x = sp._f32(x).reshape(-1)
    return np.clip(np.floor(np.nan_to_num(x, nan=0.0, posinf=255, neginf=0)), 0, 255).astype(np.uint8)
def mut_sf(rgb, tabs, C):
    o = orig["sigfuse_port"](rgb, tabs, C); return [x * F(1.0000001) for x in o]
MUTS = {"rstep_nan": ("leaf_rstep", mut_rstep_nan), "rinv_exactdiv": ("leaf_rinv", mut_rinv_exactdiv), "recip_div": ("leaf_recip", mut_recip_div),
        "blend_swap": ("leaf_blend", mut_blend_swap), "box_mode": ("strip_box3", mut_box_mode), "rms_noscale": ("strip_rms3", mut_rms_s),
        "sel2_noflag": ("leaf_sel2", mut_sel2_flag), "ixform_swap": ("ixform_neon", mut_ix), "u8_floor": ("leaf_u8", mut_u8), "sigfuse_ulp": ("sigfuse_port", mut_sf)}
if __name__ == "__main__":
    cs = V.cases(7770021)
    gs = {}
    # 本家出力はキャッシュ済み。V.genuine はキャッシュがあれば nvh を呼ばない
    from concurrent.futures import ThreadPoolExecutor
    with ThreadPoolExecutor(3) as ex: g = list(ex.map(V.genuine, cs))
    for name, (attr, fn) in MUTS.items():
        setattr(sp, attr, fn)
        # ELEM 辞書の lambda は sp.leaf_* を名前で引くので差し替えが効く。_fabs などは未変更
        diff = tot = 0
        for c, gg in zip(cs, g):
            nm = c[1] if c[0] in ("elem", "strip3") else c[0]
            rel = {"leaf_rstep": ("elem", "rstep"), "leaf_rinv": ("elem", "rinv"), "leaf_recip": ("elem", "recip"), "leaf_blend": ("elem", "blend"),
                   "strip_box3": ("strip3", "box"), "strip_rms3": ("strip3", "sq"), "leaf_sel2": ("sel2", None), "ixform_neon": ("ixform", None),
                   "leaf_u8": ("elem", "u8"), "sigfuse_port": ("sigfuse", None)}[attr]
            if c[0] != rel[0] or (rel[1] and c[1] != rel[1]): continue
            tot += 1
            if not V.cmp_prefix(c, gg, V.port(c)): diff += 1
        print("mutant %-14s detected in %d / %d cases" % (name, diff, tot))
        setattr(sp, attr, orig[attr])
