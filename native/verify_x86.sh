#!/bin/bash
# F4: the x86_64 build of the bit-exact ports against the arm64 build, byte for byte (no Python in the
# comparison), on the inputs of the Python verifications (NaN / inf / signalling-NaN cases included).
#
#   native/verify_x86.sh [--only band,curve,model,flat] [--ref DIR] [--keep] [--no-capture]
#
# 1. make all portable x86: <name> (arm64, inline asm), <name>_p (arm64, -DNV_PORTABLE),
#    <name>_x86 (x86_64, -arch x86_64; Rosetta 2 runs it) for nv_band_test, nv_curve_test, nv_flat_test,
#    nv_model_test.
# 2. capture: native/verify_band.py / verify_curve.py / verify_model.py / verify_flat.py (and
#    verify_flat_cancel.py) run with the binary replaced by a wrapper that keeps every input blob and
#    the command line, then runs the arm64 binary (so the Python check runs as usual).
# 3. replay: every captured command on <name>, <name>_p and <name>_x86 (same input blob, same
#    arguments, same NV_FLAT_CANCEL_AT); the outputs must be byte-identical.  --ref DIR: also the
#    binaries of the same names in DIR (e.g. a build before a refactoring; a command that DIR's binary
#    does not know is counted as skipped).
# Work directory: $NV_X86_WORK (default work/omo/wp_native_portable/x86); the captured blobs are
# removed at the end unless --keep.  --no-capture replays an earlier --keep capture.
set -u
HERE=$(cd "$(dirname "$0")" && pwd)
ROOT=$(dirname "$HERE")
WORK=${NV_X86_WORK:-$ROOT/work/omo/wp_native_portable/x86}
ONLY=band,curve,model,flat
REF=
KEEP=0
CAPTURE=1
while [ $# -gt 0 ]; do
    case "$1" in
        --only) ONLY=$2; shift 2 ;;
        --ref) REF=$(cd "$2" && pwd); shift 2 ;;
        --keep) KEEP=1; shift ;;
        --no-capture) CAPTURE=0; KEEP=1; shift ;;
        *) echo "unknown argument $1"; exit 2 ;;
    esac
done
want() { case ",$ONLY," in *",$1,"*) return 0 ;; esac; return 1; }

make -C "$HERE" all portable x86 >/dev/null || { echo "build failed"; exit 2; }
mkdir -p "$WORK"

# ---------------------------------------------------------------- capture
wrapper() {   # wrapper <tool> <real binary>: prints the wrapper's path
    local w="$WORK/wrap_$1.sh"
    cat > "$w" <<EOF
#!/bin/bash
# captured by native/verify_x86.sh: keeps the input blob and the command line, then runs the binary
args=("\$@")
n=\${#args[@]}
j=()
if [ \$n -ge 2 ] && [ "\${args[\$((n-2))]}" = "-j" ]; then j=(-j "\${args[\$((n-1))]}"); n=\$((n-2)); fi
d=\$(mktemp -d "$WORK/cap/$1/c.XXXXXXXX")
ln -f "\${args[\$((n-2))]}" "\$d/in.bin" 2>/dev/null || cp "\${args[\$((n-2))]}" "\$d/in.bin"
for a in "\${args[@]:0:\$((n-2))}"; do printf '%s\n' "\$a"; done > "\$d/pre"
printf '%s\n' "\${j[@]+"\${j[@]}"}" > "\$d/jflag"
printf '%s' "\${NV_FLAT_CANCEL_AT-}" > "\$d/cancel_at"
exec "$2" "\$@"
EOF
    chmod +x "$w"
    echo "$w"
}

if [ $CAPTURE = 1 ]; then
    rm -rf "$WORK/cap"
    for t in band curve model flat; do mkdir -p "$WORK/cap/$t"; done
    if want band; then
        echo "== capture: verify_band.py"
        python3 "$HERE/verify_band.py" --jobs 3 --exe "$(wrapper band "$HERE/nv_band_test")" | tail -3
    fi
    if want curve; then
        echo "== capture: verify_curve.py"
        NV_CURVE_EXE=$(wrapper curve "$HERE/nv_curve_test") python3 "$HERE/verify_curve.py" --jobs 3 | tail -2
    fi
    if want model; then
        echo "== capture: verify_model.py"
        python3 "$HERE/verify_model.py" --jobs 3 --exe "$(wrapper model "$HERE/nv_model_test")" | tail -5
    fi
    if want flat; then
        echo "== capture: verify_flat.py, verify_flat_cancel.py"
        W=$(wrapper flat "$HERE/nv_flat_test")
        NV_FLAT_EXE=$W python3 "$HERE/verify_flat.py" --jobs 3 | tail -2
        NV_FLAT_EXE=$W python3 "$HERE/verify_flat_cancel.py" --jobs 3 | tail -2
    fi
fi

# ---------------------------------------------------------------- replay
total=0
bad=0
skipped=0
for t in band curve model flat; do
    want "$t" || continue
    bin="$HERE/nv_${t}_test"
    n=0; nb=0; nskip=0; bytes=0; nnan=0
    t0=$(date +%s)
    for d in "$WORK/cap/$t"/c.*; do
        [ -d "$d" ] || continue
        pre=()
        while IFS= read -r a; do pre+=("$a"); done < "$d/pre"
        jf=()
        while IFS= read -r a; do [ -n "$a" ] && jf+=("$a"); done < "$d/jflag"
        ca=$(cat "$d/cancel_at")
        vars=("" _p _x86)
        [ -n "$REF" ] && vars+=(ref)
        for v in "${vars[@]}"; do
            if [ "$v" = ref ]; then b="$REF/nv_${t}_test"; else b="$bin$v"; fi
            rm -f "$d/out$v"
            if [ -n "$ca" ]; then
                NV_FLAT_CANCEL_AT=$ca "$b" "${pre[@]+"${pre[@]}"}" "$d/in.bin" "$d/out$v" "${jf[@]+"${jf[@]}"}" 2>"$d/err$v" >/dev/null
            else
                env -u NV_FLAT_CANCEL_AT "$b" "${pre[@]+"${pre[@]}"}" "$d/in.bin" "$d/out$v" "${jf[@]+"${jf[@]}"}" 2>"$d/err$v" >/dev/null
            fi
            echo $? > "$d/rc$v"
        done
        n=$((n + 1))
        same=1
        for v in _p _x86; do
            if [ "$(cat "$d/rc$v")" != "$(cat "$d/rc")" ] || ! cmp -s "$d/out" "$d/out$v"; then
                same=0
                echo "  DIFFER [$t] $(basename "$d") ${pre[*]-} ${jf[*]-} cancel_at=$ca: nv_${t}_test$v (rc $(cat "$d/rc$v") vs $(cat "$d/rc"); $(cmp "$d/out" "$d/out$v" 2>&1 | head -1))"
            fi
        done
        if [ -n "$REF" ]; then
            if [ "$(cat "$d/rcref")" != 0 ] || [ ! -s "$d/outref" ]; then
                nskip=$((nskip + 1))
            elif ! cmp -s "$d/out" "$d/outref"; then
                same=0
                echo "  DIFFER [$t] $(basename "$d") ${pre[*]-}: ref ($(cmp "$d/out" "$d/outref" 2>&1 | head -1))"
            fi
        fi
        [ $same = 1 ] || nb=$((nb + 1))
        bytes=$((bytes + $(stat -f %z "$d/out" 2>/dev/null || echo 0)))
        [ $KEEP = 1 ] || rm -f "$d"/out*
    done
    t1=$(date +%s)
    msg="$t: $((n - nb))/$n commands byte-identical (arm64 asm / arm64 portable / x86_64"
    [ -n "$REF" ] && msg="$msg / ref: $((n - nskip)) compared, $nskip skipped"
    echo "$msg; $bytes output bytes per binary; $((t1 - t0)) s)"
    total=$((total + n)); bad=$((bad + nb)); skipped=$((skipped + nskip))
done
[ $KEEP = 1 ] || rm -rf "$WORK/cap" "$WORK"/wrap_*.sh
echo "total: $((total - bad))/$total commands identical$( [ $bad = 0 ] && echo ': ALL IDENTICAL' || echo ": $bad DIFFER")"
[ $bad = 0 ]
