"""FUN_009c3bc4: the white-noise generator of the reference model (FUN_009b4428: 256 x 256 values,
seed 0x12345678).

The loop body (0x9c3bd0..0x9c3cc8) is integer mixing of a 32-bit state and the float32 sum of eight 5-bit
fields.  It is executed by a small interpreter over the instruction text of fn_0x9c3bc4 (otool -tv) (the
extracted disassembly), so the register roles stay exactly as in the binary.  The sums are small integers
(exact in float32).  noise(n, seed) -> float32[n]."""
import os
import re

import numpy as np

F32 = np.float32
M = 0xffffffff
# 0x9c3bc4.. of the arm64 slice (otool -tv), embedded so that repro/ needs no analysis files
_DIS_TEXT = """\
9c3bc4	cmp	x1, #0x1
9c3bc8	b.lt	0x9c3cd8
9c3bcc	ldr	w8, [x2]
9c3bd0	lsr	w9, w8, #16
9c3bd4	add	w9, w9, w8
9c3bd8	add	w9, w9, w9, lsl #8
9c3bdc	add	w9, w9, w8, lsr #24
9c3be0	extr	w10, w9, w8, #0x10
9c3be4	ubfx	w8, w8, #16, #5
9c3be8	ucvtf	s0, w8
9c3bec	add	w8, w10, w9, uxth
9c3bf0	add	w8, w8, w8, lsl #8
9c3bf4	lsr	w10, w9, #8
9c3bf8	add	w8, w8, w10, uxtb
9c3bfc	and	w10, w9, #0x1f
9c3c00	bfi	w9, w8, #16, #16
9c3c04	ucvtf	s1, w10
9c3c08	fadd	s0, s0, s1
9c3c0c	add	w9, w9, w8, uxth
9c3c10	add	w9, w9, w9, lsl #8
9c3c14	lsr	w10, w8, #8
9c3c18	add	w9, w9, w10, uxtb
9c3c1c	and	w10, w8, #0x1f
9c3c20	bfi	w8, w9, #16, #16
9c3c24	ucvtf	s1, w10
9c3c28	fadd	s0, s0, s1
9c3c2c	add	w8, w8, w9, uxth
9c3c30	add	w8, w8, w8, lsl #8
9c3c34	lsr	w10, w9, #8
9c3c38	add	w8, w8, w10, uxtb
9c3c3c	and	w10, w9, #0x1f
9c3c40	bfi	w9, w8, #16, #16
9c3c44	ucvtf	s1, w10
9c3c48	fadd	s0, s0, s1
9c3c4c	add	w9, w9, w8, uxth
9c3c50	add	w9, w9, w9, lsl #8
9c3c54	lsr	w10, w8, #8
9c3c58	add	w9, w9, w10, uxtb
9c3c5c	and	w10, w8, #0x1f
9c3c60	bfi	w8, w9, #16, #16
9c3c64	ucvtf	s1, w10
9c3c68	fadd	s0, s0, s1
9c3c6c	add	w8, w8, w9, uxth
9c3c70	add	w8, w8, w8, lsl #8
9c3c74	lsr	w10, w9, #8
9c3c78	add	w10, w8, w10, uxtb
9c3c7c	and	w8, w9, #0x1f
9c3c80	bfi	w9, w10, #16, #16
9c3c84	ucvtf	s1, w8
9c3c88	fadd	s0, s0, s1
9c3c8c	add	w8, w9, w10, uxth
9c3c90	add	w8, w8, w8, lsl #8
9c3c94	lsr	w9, w10, #8
9c3c98	add	w8, w8, w9, uxtb
9c3c9c	and	w9, w10, #0x1f
9c3ca0	ucvtf	s1, w9
9c3ca4	fadd	s0, s0, s1
9c3ca8	add	w9, w8, w10
9c3cac	add	w9, w9, w9, lsl #8
9c3cb0	lsr	w10, w8, #8
9c3cb4	add	w9, w9, w10, uxtb
9c3cb8	and	w10, w8, #0x1f
9c3cbc	bfi	w8, w9, #16, #16
9c3cc0	ucvtf	s1, w10
9c3cc4	fadd	s0, s0, s1
9c3cc8	str	s0, [x0], #0x4
9c3ccc	subs	x1, x1, #0x1
9c3cd0	b.ne	0x9c3bd0
9c3cd4	str	w8, [x2]
9c3cd8	ret
9c3cdc	stp	x20, x19, [sp, #-0x20]!
9c3ce0	stp	x29, x30, [sp, #0x10]
9c3ce4	add	x29, sp, #0x10
"""


def _body():
    prog = []
    on = False
    for line in _DIS_TEXT.splitlines():
        addr, _, ins = line.partition("\t")
        a = int(addr, 16)
        if a == 0x9c3bd0:
            on = True
        if on:
            prog.append(ins.strip())
        if a == 0x9c3cc4:
            break
    return prog


_PROG = None


def _src(regs, ops):
    """Value of the second source operand with an optional shift / extend."""
    v = regs[ops[0]] if ops[0] in regs else int(ops[0].lstrip("#"), 0)
    if len(ops) > 1:
        kind = ops[1].split()[0]
        if kind == "lsl":
            v = (v << int(ops[1].split("#")[1], 0)) & M
        elif kind == "lsr":
            v = v >> int(ops[1].split("#")[1], 0)
        elif kind == "uxth":
            v &= 0xffff
        elif kind == "uxtb":
            v &= 0xff
        else:
            raise NotImplementedError(ops[1])
    return v


def _step(regs, ins):
    op, rest = ins.split(None, 1)
    a = [t.strip() for t in rest.split(",")]
    d = a[0]
    if op == "lsr":
        regs[d] = regs[a[1]] >> int(a[2].lstrip("#"), 0)
    elif op == "add":
        regs[d] = (regs[a[1]] + _src(regs, a[2:])) & M
    elif op == "and":
        regs[d] = regs[a[1]] & int(a[2].lstrip("#"), 0)
    elif op == "extr":
        sh = int(a[3].lstrip("#"), 0)
        regs[d] = ((regs[a[1]] << (32 - sh)) | (regs[a[2]] >> sh)) & M
    elif op == "ubfx":
        lsb, w = int(a[2].lstrip("#"), 0), int(a[3].lstrip("#"), 0)
        regs[d] = (regs[a[1]] >> lsb) & ((1 << w) - 1)
    elif op == "bfi":
        lsb, w = int(a[2].lstrip("#"), 0), int(a[3].lstrip("#"), 0)
        mask = ((1 << w) - 1) << lsb
        regs[d] = (regs[d] & ~mask & M) | ((regs[a[1]] << lsb) & mask)
    elif op == "ucvtf":
        regs[d] = F32(regs[a[1]])
    elif op == "fadd":
        regs[d] = F32(regs[a[1]] + regs[a[2]])
    elif op in ("str", "subs", "b.ne"):
        pass
    else:
        raise NotImplementedError(ins)


def noise_ref(n, seed=0x12345678):
    """The interpreter of the original port (kept as the reference the compiled loop is checked
    against)."""
    global _PROG
    if _PROG is None:
        _PROG = _body()
    regs = {"w8": seed & M}
    out = np.empty(n, F32)
    for i in range(n):
        for ins in _PROG:
            if ins.startswith("str"):
                continue
            _step(regs, ins)
        out[i] = regs["s0"]
    return out


# ------------------------------------------------------------------------------ compiled loop
# The instruction text of _DIS_TEXT is translated once into Python statements with exactly the
# semantics of _step (the interpreter above); the generated loop runs the same integer / fadd
# chain on local variables.  Verified against noise_ref (see verify/check_refnoise.py).


def _gen_src(prog):
    lines = ["def _loop(n, seed, _F32, _M, _np, _F32t):", "    w8 = seed & _M", "    s0 = _F32(0)",
             "    out = _np.empty(n, _F32t)", "    for i in range(n):"]
    for ins in prog:
        op, _, rest = ins.partition("\t")
        if not _:
            op, _, rest = ins.partition(" ")
        if op in ("str", "subs", "b.ne"):
            continue
        a = [t.strip() for t in rest.split(",")]
        d = a[0]
        if op == "lsr":
            lines.append(f"        {d} = {a[1]} >> {int(a[2].lstrip('#'), 0)}")
        elif op == "add":
            src = a[2] if len(a) > 2 else None
            if src.startswith("#"):
                e = f"{a[1]} + {int(src.lstrip('#'), 0)}"
            else:
                e = f"{a[1]} + {src}"
                if len(a) > 3:
                    kind = a[3]
                    if kind.startswith("lsl"):
                        e = f"{a[1]} + (({src} << {int(kind.split('#')[1], 0)}) & _M)"
                    elif kind.startswith("lsr"):
                        e = f"{a[1]} + ({src} >> {int(kind.split('#')[1], 0)})"
                    elif kind == "uxth":
                        e = f"{a[1]} + ({src} & 0xffff)"
                    elif kind == "uxtb":
                        e = f"{a[1]} + ({src} & 0xff)"
                    else:
                        raise NotImplementedError(kind)
            lines.append(f"        {d} = ({e}) & _M")
        elif op == "and":
            lines.append(f"        {d} = {a[1]} & {int(a[2].lstrip('#'), 0)}")
        elif op == "extr":
            sh = int(a[3].lstrip("#"), 0)
            lines.append(f"        {d} = (({a[1]} << {32 - sh}) | ({a[2]} >> {sh})) & _M")
        elif op == "ubfx":
            lsb, w = int(a[2].lstrip("#"), 0), int(a[3].lstrip("#"), 0)
            lines.append(f"        {d} = ({a[1]} >> {lsb}) & {(1 << w) - 1}")
        elif op == "bfi":
            lsb, w = int(a[2].lstrip("#"), 0), int(a[3].lstrip("#"), 0)
            msk = ((1 << w) - 1) << lsb
            lines.append(f"        {d} = ({d} & {M ^ msk}) | (({a[1]} << {lsb}) & {msk})")
        elif op == "ucvtf":
            lines.append(f"        {d} = _F32({a[1]})")
        elif op == "fadd":
            lines.append(f"        {d} = _F32({a[1]} + {a[2]})")
        else:
            raise NotImplementedError(ins)
    lines.append("        out[i] = s0")
    lines.append("    return out")
    return "\n".join(lines)


_LOOP = None
_CACHE = {}


def noise(n, seed=0x12345678):
    """FUN_009c3bc4 (float32[n]); the compiled loop of the instruction text, cached per (n, seed)."""
    global _PROG, _LOOP
    key = (int(n), seed & M)
    if key in _CACHE:
        return _CACHE[key]
    if _PROG is None:
        _PROG = _body()
    if _LOOP is None:
        _g = {"np": np}
        exec(compile(_gen_src(_PROG), "<nv_refnoise gen>", "exec"), _g)  # noqa: S102
        _LOOP = _g["_loop"]
    return _CACHE.setdefault(key, _LOOP(n, seed, F32, M, np, F32))
