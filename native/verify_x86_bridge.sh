#!/bin/bash
# G9: libnvcore_x86.dylib (x86_64, NV_PORTABLE by architecture; Rosetta 2) and libnvcore_p.dylib (arm64,
# -DNV_PORTABLE) against libnvcore.dylib (arm64) on the calls verify_bridge.py makes, byte for byte.  No x86_64
# numpy is needed: the arm64 run records its ctypes calls and an x86_64 Python (standard library only) replays them.
#
#   native/verify_x86_bridge.sh [--only GROUP,..] [--quick] [--export] [--every N] [--keep] [--no-capture]
#                               [--work DIR] [--py86 PYTHON] [--allow-known]
#
# 1. make lib lib_portable lib_x86.
# 2. capture: native/verify_bridge.py (the groups of --only, default all; --quick / --export passed on) with
#    PYTHONPATH=native/x86cap: native/x86_bridge_cap.py records every nvc_* call of every process (inputs,
#    the before-call bytes of the output buffers, the cancel answers; the after-call hashes).  verify_bridge.py
#    must still report ALL MATCH (Python vs arm64).
# 3. replay (native/x86_bridge_replay.py) on libnvcore.dylib (arm64: the replay itself must reproduce the
#    recording), libnvcore_p.dylib (arm64 portable) and libnvcore_x86.dylib (--py86, default the python.org
#    3.12 universal build under `arch -x86_64`); every buffer after the call, the return value and the number of
#    cancel queries must equal the recording.  --every N replays every N-th call only (Rosetta is slow).
# --allow-known: the x86_64 replay also passes when every difference is of the two known kinds
#    (native/x86_bridge_known.py; the differing calls are replayed on arm64 with --dump-all and compared word by word):
#    (a) nvc_temporal (also inside the export): its float32 out / rv planes only, finite words (no NaN / inf)
#    1..MAX_ULP (8, the largest observed) ULP apart - libm expf, W2c; (b) nvc_nlc_image: its float32 output only,
#    NaN words whose sign / payload differ.  Any other difference fails, and so does an x86_64 replay that ended
#    with an error (only its exit code 3, "calls differ", is classified; earlier results are removed first).
#    The arm64 and arm64 portable replays are always strict.
# A missing capture, a replay of no call, or a group of --only (default: the groups the capture ran, kept in
# <work>/cap/groups.txt; else all of verify_bridge.py's, plus export with --export) without a recorded call fails.
# Work directory: $NV_X86_WORK or --work (default work/omo/wp_qtui_fix/x86_bridge); the capture is removed at
# the end unless --keep.  --no-capture replays an earlier --keep capture.  Results: <work>/replay_*.json.
set -u
HERE=$(cd "$(dirname "$0")" && pwd)
ROOT=$(dirname "$HERE")
WORK=${NV_X86_WORK:-$ROOT/work/omo/wp_qtui_fix/x86_bridge}
PY86=/Library/Frameworks/Python.framework/Versions/3.12/bin/python3.12
ONLY=
VB=()
EVERY=1
KEEP=0
CAPTURE=1
ALLOW_KNOWN=0
KNOWN_ONLY=
while [ $# -gt 0 ]; do
    case "$1" in
        --only) ONLY=$2; shift 2 ;;
        --quick|--export) VB+=("$1"); shift ;;
        --every) EVERY=$2; shift 2 ;;
        --keep) KEEP=1; shift ;;
        --no-capture) CAPTURE=0; KEEP=1; shift ;;
        --work) WORK=$2; shift 2 ;;
        --py86) PY86=$2; shift 2 ;;
        --allow-known) ALLOW_KNOWN=1; shift ;;
        *) echo "unknown argument $1"; exit 2 ;;
    esac
done
[ -n "$ONLY" ] && VB+=(--only "$ONLY")
EXPORT=0
for a in "${VB[@]+"${VB[@]}"}"; do [ "$a" = --export ] && EXPORT=1; done

make -C "$HERE" lib lib_portable lib_x86 >/dev/null || { echo "build failed"; exit 2; }
arch -x86_64 "$PY86" -c "import ctypes, platform; assert platform.machine() == 'x86_64'" ||
    { echo "no x86_64 Python ($PY86)"; exit 2; }
mkdir -p "$WORK"

if [ $CAPTURE = 1 ]; then
    rm -rf "$WORK/cap"
    mkdir -p "$WORK/cap"
    echo "== capture: verify_bridge.py ${VB[*]-}"
    NV_X86_CAP="$WORK/cap" PYTHONPATH="$HERE/x86cap${PYTHONPATH:+:$PYTHONPATH}" \
        python3 "$HERE/verify_bridge.py" --tmp "$WORK/bridge_tmp" "${VB[@]+"${VB[@]}"}" > "$WORK/capture.log" 2>&1
    rc=$?
    g=${ONLY:-curve,profile,aft,frame,cancel,fallback,prof5,ui,depth}
    [ $EXPORT = 1 ] && g="$g,export"
    echo "$g" > "$WORK/cap/groups.txt"
    tail -14 "$WORK/capture.log"
    [ $rc = 0 ] || { echo "verify_bridge.py failed (rc $rc) during the capture: see $WORK/capture.log"; exit 1; }
fi

if ! ls "$WORK/cap"/cap_*.pkl >/dev/null 2>&1; then
    echo "FAIL: no capture in $WORK/cap (run without --no-capture, or with --keep first)"
    exit 1
fi
if [ -n "$ONLY" ]; then
    REQ=$ONLY
    [ $EXPORT = 1 ] && REQ="$REQ,export"
elif [ -f "$WORK/cap/groups.txt" ]; then
    REQ=$(cat "$WORK/cap/groups.txt")
else
    REQ=curve,profile,aft,frame,cancel,fallback,prof5,ui,depth
    [ $EXPORT = 1 ] && REQ="$REQ,export"
fi
echo "== required groups: $REQ"
# results of this run only: an earlier run's JSON / dumps must never be classified (or read) again
rm -rf "$WORK"/replay_*.json "$WORK/dump_x86" "$WORK/dump_arm64"
bad=0
echo "== replay: arm64 (libnvcore.dylib, the recording itself)"
python3 "$HERE/x86_bridge_replay.py" --cap "$WORK/cap" --lib "$HERE/libnvcore.dylib" --every "$EVERY" \
    --require-groups "$REQ" --out "$WORK/replay_arm64.json" || bad=$((bad + 1))
echo "== replay: arm64 portable (libnvcore_p.dylib)"
python3 "$HERE/x86_bridge_replay.py" --cap "$WORK/cap" --lib "$HERE/libnvcore_p.dylib" --every "$EVERY" \
    --require-groups "$REQ" --out "$WORK/replay_portable.json" || bad=$((bad + 1))
echo "== replay: x86_64 (libnvcore_x86.dylib, Rosetta 2)"
arch -x86_64 "$PY86" "$HERE/x86_bridge_replay.py" --cap "$WORK/cap" --lib "$HERE/libnvcore_x86.dylib" \
    --every "$EVERY" --require-groups "$REQ" --out "$WORK/replay_x86.json" --dump "$WORK/dump_x86"
rc=$?
# x86_bridge_replay.py: 0 identical, 3 calls differ (a finished comparison), 2 no capture / no call / a group
# missing, 1 a Python exception; only 3 is classified
if [ $rc = 3 ] && [ $ALLOW_KNOWN = 1 ] && [ -s "$WORK/replay_x86.json" ]; then
    echo "== --allow-known: arm64 replay of the differing calls, word-level classification"
    SEL=$(python3 -c "import json, sys; print(','.join(b['call'] for b in json.load(open(sys.argv[1]))['bad']))" \
        "$WORK/replay_x86.json")
    rm -rf "$WORK/dump_arm64"
    if [ -n "$SEL" ] && python3 "$HERE/x86_bridge_replay.py" --cap "$WORK/cap" --lib "$HERE/libnvcore.dylib" \
            --select "$SEL" --dump "$WORK/dump_arm64" --dump-all --out "$WORK/replay_arm64_sel.json" >/dev/null; then
        python3 "$HERE/x86_bridge_known.py" --x86 "$WORK/replay_x86.json" --x86-dump "$WORK/dump_x86" \
            --arm-dump "$WORK/dump_arm64" && { rc=0; KNOWN_ONLY=1; }
    fi
fi
[ $rc = 0 ] || bad=$((bad + 1))
[ $KEEP = 1 ] || rm -rf "$WORK/cap" "$WORK/dump_x86" "$WORK/dump_arm64"
if [ $bad = 0 ]; then echo "ALL IDENTICAL${KNOWN_ONLY:+ (x86_64: known differences only, --allow-known)}"
else echo "$bad REPLAYS FAILED (calls differ, an error, or no capture / no call / a required group missing)"; fi
[ $bad = 0 ]
