"""Profile Quality の草案 (静的解析 QUALITY.md の式。本家は実行しない)。

    quality_from_profile(blob, g=None) -> dict
        blob: nv_profile.build_profile の 0xc300 バイト (本家の DeviceNoiseProfile の先頭と同じ配置)
        g   : 輝度曲線 (+0xbf40) の項。本家 RS 0xa4b03c / Demo 0x9bb748 の戻り値で、入口に難読化があるため
              読んでいない (未確定)。None なら g を含まない部分 R と、g ∈ [0, 1] と仮定したときの範囲を返す。

RS 0xa4acf8 (Demo 0x9bb3e0) の観測した式 (難読化ブロックは解釈していない):
    p = profile
    if byte p[0xbf34] == 0: return 0
    s11 = 0.5*g(p+0xbf40) + 0.5
    for i in 0..2 (変換面 3..5 = 色モード 1 で Y, Cr, Cb):
        f_i = FUN_009b096c(p + 0x4c + (3+i)*0x1f54, luma=DAT[mode*3+i])   # 平坦度 (nv_flat.flatness)
        u_i = FUN_009d49b4(f_i, 2.0, -0.25)                               # = clamp(2.5 - f_i, 0, 1)
        if luma flag == 0: u_i = 0.5*u_i + 0.5
    P = ((u0+0.5)*(u1+0.5)*(u2+0.5) + 0.25) / 3.625                        # 最大 1
    q = s11 * P
    W, H = int32 p[0xc2c0], p[0xc2c4]
    if W and H: m = min(W, H); S = 0 (m<16) / k + (m-T[k-1])/(T[k]-T[k-1]) / 5 (m>=256), T = 16,32,64,128,256
    else: S = float(int32 p[0xbf38])
    q *= S / 5
    for c in 0..2: if byte p[0xbf3c+c]: q *= 0.5
    表示: Profile Viewer = qRound(double(q)*100) "%"、Simple/Advanced = qRound(q*100.0f) "%"
"""
import os
import struct
import sys

import numpy as np

_HERE = os.path.dirname(os.path.abspath(__file__))
_REPRO = os.path.normpath(os.path.join(_HERE, "..", "..", "..", "..", "..", "repro"))
if _REPRO not in sys.path:
    sys.path.insert(0, _REPRO)
sys.dont_write_bytecode = True

import nv_flat as nf  # noqa: E402

F32 = np.float32
U32 = np.uint32
MODEL = 0x1f54
SIZE_T = (16, 32, 64, 128, 256)                  # RS 0x162c580 / Demo 0x1488128 (int64 x5, 静的に読んだ値)
LUMA_FLAGS = (1, 1, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0)  # RS 0x1622348 / Demo 0x147dfc8 (u8, mode*3 + i)
HALF = F32(0.5)


def genuine_g(blob):
    """RS 0xa4b03c / Demo 0x9bb748 (2026-10-26 統括が解読。入口と末尾の難読化ブロックは大域語の
    記帳だけで数値 s0 に触れない。RS と Demo で同じオフセット・同じ定数・同じ分岐構造)。

    x0 = p + 0xbf40、k = 0..8 (x8 = 0x20k)、チャンネル c = 0,1,2 (+0x120c) の順に 1つずつ加算する。
    ビン (c,k) の基点 o = 0xbf44 + 0x120c + 0x20k として:
        byte o+0x1e != 0            -> s -= 1/27   (レベル > 3.0 の旗)
        elif byte o+0x1d != 0       -> s -= 1/27   (クリップ旗。AFT の curve_post が 0 に戻す)
        else:
            f32 o+0x4 == 0          -> s -= 1/81   (段係数の語が 0)
            elif i32 o+0x1c != 0    -> s += 1/27   (AFT のマーカー旗 o+0x1c)
            elif byte o+0x20 != 0   -> s += 1/81   (次のビンの語0 の下位バイト。ch2 k=8 では 0xc2a4 = LZMA 成功旗)
            else                    -> +0
    戻り値 = fmax(s, 0.0)。0xbf40 のモード語は読まない。
    """
    b = bytes(blob)
    NEG27 = F32(np.frombuffer(struct.pack("<I", 0xBD17B426), F32)[0])   # -1/27
    POS27 = F32(np.frombuffer(struct.pack("<I", 0x3D17B426), F32)[0])   # +1/27
    NEG81 = F32(np.frombuffer(struct.pack("<I", 0xBC4A4588), F32)[0])   # -1/81
    POS81 = F32(np.frombuffer(struct.pack("<I", 0x3C4A4588), F32)[0])   # +1/81
    f32 = np.frombuffer(b, F32)
    s = F32(0.0)
    for k in range(9):
        for c in range(3):
            o = (0xbf44 + 0x120 * c + 0x20 * k) // 4        # 語単位
            ob = 0xbf44 + 0x120 * c + 0x20 * k              # バイト単位
            if b[ob + 0x1e] != 0:
                s = F32(s + NEG27)
            elif b[ob + 0x1d] != 0:
                s = F32(s + NEG27)
            elif f32[o + 1] == F32(0.0):                    # 語 1 = o+0x4 (段係数の先頭)
                s = F32(s + NEG81)
            elif struct.unpack_from("<I", b, ob + 0x1c)[0] != 0:
                s = F32(s + POS27)
            elif b[ob + 0x20] != 0:
                s = F32(s + POS81)
    return F32(max(s, F32(0.0))) if s > F32(0.0) else F32(0.0)   # fmax s0, 0.0 (NaN -> 0)


def penalty(x, s=F32(2.0), c=F32(-0.25)):
    """FUN_009d49b4(x, s, c) (Demo。aft_port/notes/AutoFineTune.md の読み。RS 0xa63938 は難読化ブロックが挿入され
    バイト列が異なる。同じ式であることは推定)。"""
    x, s, c = F32(x), F32(s), F32(c)
    if s == F32(0.0):
        return F32(1.0) if F32(x * c) > F32(0.0) else F32(0.0)
    a = F32(F32(F32(F32(0.5) / s) / c) * x)
    b = F32(F32(F32(c + F32(-1.0)) * F32(0.5)) / c)
    v = F32(a + b)
    if v < F32(0.0):
        return F32(0.0)
    if v > F32(1.0):
        return F32(1.0)
    return v


def size_term(W, H, fallback_class):
    if W == 0 or H == 0:
        return F32(fallback_class)
    m = min(W, H)
    if m < SIZE_T[0]:
        return F32(0.0)
    if m >= SIZE_T[4]:
        return F32(5.0)
    k = next(k for k in range(1, 5) if m < SIZE_T[k])
    lo, hi = SIZE_T[k - 1], SIZE_T[k]
    return F32(F32(F32(m - lo) / F32(hi - lo)) + F32(k))


def components(blob):
    b = bytes(blob)
    if b[0xbf34] == 0:
        return None
    mode = struct.unpack_from("<i", b, 0x48)[0]
    w = np.frombuffer(b, U32)
    flat, u = [], []
    for i in range(3):
        off = (0x4c + (3 + i) * MODEL) // 4
        luma = LUMA_FLAGS[mode * 3 + i]
        f = nf.flatness(w[off:off + 55], bool(luma))
        v = penalty(f)
        if luma == 0:
            v = F32(F32(v * HALF) + HALF)
        flat.append(float(f))
        u.append(v)
    P = F32(F32(F32(F32(u[0] + HALF) * F32(u[1] + HALF)) * F32(u[2] + HALF)) + F32(0.25))
    P = F32(P / F32(3.625))
    W, H = struct.unpack_from("<ii", b, 0xc2c0)
    cls = struct.unpack_from("<i", b, 0xbf38)[0]
    S = size_term(W, H, cls)
    clips = [b[0xbf3c + c] != 0 for c in range(3)]
    return dict(mode=mode, flatness=flat, u=[float(x) for x in u], P=float(P), W=W, H=H, S=float(S),
                clips=clips, F32_P=P, F32_S=S)


def _finish(q, clips):
    for c in clips:
        if c:
            q = F32(q * HALF)
    return q


def quality_from_profile(blob, g=None):
    """g が数値なら本家と同じ演算順の q (float32) を返す。None なら R (= s11 を除いた部分) と範囲。"""
    c = components(blob)
    if c is None:
        return dict(q=0.0)
    P, S = c["F32_P"], c["F32_S"]
    out = {k: v for k, v in c.items() if not k.startswith("F32_")}
    if g is not None:
        s11 = F32(F32(F32(g) * HALF) + HALF)
        q = F32(s11 * P)
        q = F32(q * F32(S / F32(5.0)))
        q = _finish(q, c["clips"])
        out.update(g=float(g), q=float(q), viewer_pct=int(np.floor(float(q) * 100.0 + 0.5)))
        return out
    R = _finish(F32(P * F32(S / F32(5.0))), c["clips"])
    out.update(R=float(R), range_if_g_in_0_1=(0.5 * float(R), float(R)))
    return out


# ------------------------------------------------------------------ 照合用 (test_v02)
def _frame(path, n, scale=1):
    import subprocess
    vf = f"select=eq(n\\,{n})"
    W, H = 1920 * scale, 1080 * scale
    if scale != 1:
        vf += f",scale={W}:{H}:flags=bicubic"
    raw = subprocess.run(["ffmpeg", "-nostdin", "-v", "error", "-i", path, "-vf", vf, "-frames:v", "1",
                          "-f", "rawvideo", "-pix_fmt", "rgb48le", "-"], check=True, capture_output=True).stdout
    return (np.frombuffer(raw, np.uint16).reshape(H, W, 3).astype(F32) / F32(257.0)).astype(F32)


def _fast_blob(rgb, area, mode=1):
    """プロファイルのうち Quality が読む語だけを作る (面 3..5 の語 0..54 は nv_band.model_levels で
    nv_model.model_full と同じ。0xbf34・0xbf38・0xbf3c・0xc2c0)。"""
    import nv_band as nb
    import nv_curve as ncv
    import nv_profile as npf
    x, y, w, h = area
    crop = rgb[y:y + h, x:x + w]
    R, G, B = (np.ascontiguousarray(crop[..., c]) for c in range(3))
    T = nf.transform(R, G, B, mode)
    buf = np.zeros(0xc300, np.uint8)
    w32 = buf.view(U32)
    w32[0x48 // 4] = mode
    for i, p in enumerate(T):
        m = np.asarray(nb.model_levels(np.ascontiguousarray(p, F32))).view(U32)[:55]
        off = (0x4c + (3 + i) * MODEL) // 4
        w32[off:off + 55] = m
    buf[0xbf34] = 1
    w32[0xbf38 // 4] = npf.size_class(min(w, h))
    for c, p in enumerate((R, G, B)):
        buf[0xbf3c + c] = bool(ncv.is_clip_genuine(p) > F32(0.33))
    struct.pack_into("<ii", buf, 0xc2c0, w, h)
    return buf.tobytes()


def main():
    import json
    import time
    import nv_curve as ncv
    import nv_profile as npf
    root = os.path.normpath(os.path.join(_REPRO, ".."))
    vid = os.path.join(root, "test_v02.mp4")
    res = {}
    # 1) 1080p n=312、本家が選ぶ領域 x832 y0 128x128 (AGENTS.md §6.5)。nv_profile の完全なプロファイル
    rgb = _frame(vid, 312)
    t = time.time()
    blob, _ = npf.build_profile(rgb, 1, (832, 0, 128, 128), with_floor=False, lzma=True)
    res["n312_1x_full_profile"] = quality_from_profile(blob.tobytes(), g=genuine_g(blob))
    res["n312_1x_full_profile"]["t"] = round(time.time() - t, 1)
    fast = _fast_blob(rgb, (832, 0, 128, 128))
    # _fast_blob は LZMA 旗 0xc2a4 を書かないので、本家同等の状態にしてから g を計算
    fast = bytearray(fast)
    fast[0xc2a4] = 1
    fast = bytes(fast)
    res["n312_1x_fast"] = quality_from_profile(fast, g=genuine_g(fast))
    res["n312_1x_fast_equal"] = (res["n312_1x_fast"]["q"] == res["n312_1x_full_profile"]["q"])
    # 2) Resolve の条件の近似: フレーム 0 を 2 倍 (bicubic) に拡大し、移植の FindFlatArea で領域を選ぶ
    for n in (0, 312):
        big = _frame(vid, n, 2)
        table = ncv.genuine_noise_curve(big)
        x0, y0, w, h, st = nf.find_flat_area_genuine(big, ncv.noise_at(table), 1)
        fb = bytearray(_fast_blob(big, (x0, y0, w, h)))
        fb[0xc2a4] = 1
        r = quality_from_profile(bytes(fb), g=genuine_g(fb))
        r.update(area=[int(x0), int(y0), int(w), int(h)], status=int(st))
        res[f"n{n}_2x_ffa"] = r
        # 同じフレームの 1080p での FindFlatArea
        small = _frame(vid, n)
        table = ncv.genuine_noise_curve(small)
        x0, y0, w, h, st = nf.find_flat_area_genuine(small, ncv.noise_at(table), 1)
        fb = bytearray(_fast_blob(small, (x0, y0, w, h)))
        fb[0xc2a4] = 1
        r = quality_from_profile(bytes(fb), g=genuine_g(fb))
        r.update(area=[int(x0), int(y0), int(w), int(h)], status=int(st))
        res[f"n{n}_1x_ffa"] = r
    out = os.path.join(_HERE, "quality_draft_results.json")
    with open(out, "w") as f:
        json.dump(res, f, indent=1, default=str)
    print(json.dumps(res, indent=1, default=str))


if __name__ == "__main__":
    main()
