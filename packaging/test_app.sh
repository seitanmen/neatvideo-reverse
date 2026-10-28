#!/bin/bash
# Automated checks of packaging/dist/MeatVideo.app (README.md). No window is shown: everything runs
# with QT_QPA_PLATFORM=offscreen and the bundle's executable is started directly (no `open`).
#   packaging/test_app.sh [OUTDIR] [--full]
#   OUTDIR  default packaging/build/test_app (settings, profiles, short lossless exports, logs)
#   --full  also the self-test with NV_NATIVE=0 (Python core; AFT in a spawned child; ~3 min)
# VIDEO=<file> (default test.mp4 next to packaging/), PYTHON=<development python3> (default python3).
set -uo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(dirname "$HERE")"
APP="$HERE/dist/MeatVideo.app"
EXE="$APP/Contents/MacOS/MeatVideo"
OUT="$HERE/build/test_app"
FULL=0
for a in "$@"; do
    case "$a" in --full) FULL=1 ;; *) OUT="$a" ;; esac
done
VIDEO="${VIDEO:-$ROOT/test.mp4}"
PYTHON="${PYTHON:-python3}"
export QT_QPA_PLATFORM=offscreen
mkdir -p "$OUT"
OUT="$(cd "$OUT" && pwd)"
VIDEO="$(cd "$(dirname "$VIDEO")" && pwd)/$(basename "$VIDEO")"
FAILS=0
pass() { echo "[PASS] $*"; }
fail() { echo "[FAIL] $*"; FAILS=$((FAILS + 1)); }
[ -x "$EXE" ] || { echo "no $EXE (run packaging/build_app.sh)" >&2; exit 2; }

snapshot() {   # contents of every file and the target of every link
    (cd "$APP" && find . -type f -print0 | sort -z | xargs -0 shasum -a 256 &&
     find . -type l | sort | while read -r l; do echo "link $l -> $(readlink "$l")"; done)
}
snapshot > "$OUT/bundle_before.sha256"

# 1. signature
if codesign --verify --deep --strict --verbose=2 "$APP" > "$OUT/codesign.txt" 2>&1; then
    pass "codesign --verify --deep --strict: $(tail -1 "$OUT/codesign.txt")"
else
    fail "codesign --verify: $(cat "$OUT/codesign.txt")"
fi

# 2. bundle diagnostics (library, ref_floor, versions, ffmpeg, a spawned child)
if "$EXE" --info > "$OUT/info.json" 2> "$OUT/info.err"; then
    "$PYTHON" - "$OUT/info.json" "$APP" <<'EOF' || FAILS=$((FAILS + 1))
import json, os, sys
d = json.load(open(sys.argv[1])); app = os.path.realpath(sys.argv[2]); fw = os.path.join(app, "Contents", "Frameworks")
c = d["child"]
checks = [
    ("frozen", d["frozen"] is True),
    ("numpy 2.1.3 / scipy 1.15.3 / Python 3.13 / PySide6 6.12 bundled",
     (d["numpy"], d["scipy"], d["python"][:4], d["pyside6"][:4]) == ("2.1.3", "1.15.3", "3.13", "6.12")),
    ("libnvcore.dylib loaded from Contents/Frameworks",
     d["native_available"] and os.path.realpath(d["native_lib"]) == os.path.join(fw, "libnvcore.dylib")),
    ("native core enabled", d["native_enabled"] is True),
    ("ref_floor.npy read from the bundle (bit-equal, not recomputed)",
     d["ref_floor_file_exists"] and d["ref_floor_equals_file"] and d["ref_floor_seconds"] < 0.5
     and os.path.realpath(d["ref_floor_path"]).startswith(app)),
    ("libSystem pow through ctypes (nv_curve)", d["libsystem_pow"] == "_FuncPtr"),
    ("ffmpeg found", bool(d["ffmpeg_dir"])),
    ("spawned child: same ref_floor path, not recomputed, same ffmpeg",
     c["ref_floor_path"] == d["ref_floor_path"] and c["ref_floor_equals"] and c["ref_floor_seconds"] < 0.5
     and c["ffmpeg_dir"] == d["ffmpeg_dir"]),
]
for name, ok in checks:
    print(f"[{'PASS' if ok else 'FAIL'}] {name}")
print(f"       lib {d['native_lib']}\n       ref_floor {d['ref_floor_path']} ({d['ref_floor_seconds']} s; child "
      f"{c['ref_floor_seconds']} s, child start {c['start_seconds']} s)\n       ffmpeg {d['ffmpeg_dir']}")
sys.exit(0 if all(ok for _, ok in checks) else 1)
EOF
else
    fail "--info: $(tail -3 "$OUT/info.err")"
fi

# 3. startup (offscreen): launch -> the main window built and shown, first event-loop turn; 3 warm runs
for i in 1 2 3; do
    t0=$(perl -MTime::HiRes=time -e 'printf "%.6f", time')
    s=$(NV_LAUNCH_TIMING=$t0 NV_UI_QT_SETTINGS="$OUT/startup.ini" "$EXE" 2>/dev/null | grep '^startup')
    t1=$(perl -MTime::HiRes=time -e 'printf "%.6f", time')
    echo "       run $i: $s (to the window), $(perl -e "printf '%.3f', $t1 - $t0") s (process start to exit)"
done

# 4. self-test with the bundled Python, native core on
"$EXE" --selftest "$VIDEO" "$OUT/st_on" > "$OUT/selftest_on.txt" 2>&1
grep -E "passed|STATS" "$OUT/selftest_on.txt" | sed 's/^/       /'
if grep -qE '^([0-9]+)/\1 passed' "$OUT/selftest_on.txt" && grep -q "native_enabled True" "$OUT/selftest_on.txt" &&
   grep -q "'fallback': 0}" "$OUT/selftest_on.txt" && ! grep -q "'native': 0," "$OUT/selftest_on.txt"; then
    pass "self-test (native on): all passed, C calls > 0, no Python fallback"
else
    fail "self-test (native on): $(grep -E 'FAIL|passed' "$OUT/selftest_on.txt" | head -5)"
fi

# 5. (--full) self-test with the Python core: AFT runs in a spawned child of the frozen app
if [ $FULL = 1 ]; then
    NV_NATIVE=0 "$EXE" --selftest "$VIDEO" "$OUT/st_off" > "$OUT/selftest_off.txt" 2>&1
    grep -E "passed|STATS|AFT cancellable" "$OUT/selftest_off.txt" | sed 's/^/       /'
    if grep -qE '^([0-9]+)/\1 passed' "$OUT/selftest_off.txt" && grep -q "native_enabled False" "$OUT/selftest_off.txt"; then
        pass "self-test (NV_NATIVE=0): all passed"
    else
        fail "self-test (NV_NATIVE=0): $(grep -E 'FAIL|passed' "$OUT/selftest_off.txt" | head -5)"
    fi
fi

# 6. exports == the development environment's (native on). The exports run as `MeatVideo -m
# nv_ui_qt.export`, frames in spawned ProcessPoolExecutor workers of the frozen app.
(cd "$ROOT/repro" && "$PYTHON" -m nv_ui_qt.selftest "$VIDEO" "$OUT/st_dev" > "$OUT/selftest_dev.txt" 2>&1)
grep -E "passed" "$OUT/selftest_dev.txt" | sed 's/^/       dev /'
for d in st_on $([ $FULL = 1 ] && echo st_off); do
    for f in export.mp4 export_tnoise.mp4; do
        if [ -s "$OUT/$d/$f" ] && cmp -s "$OUT/$d/$f" "$OUT/st_dev/$f"; then
            pass "$d/$f == development export, byte for byte ($(stat -f %z "$OUT/$d/$f") B)"
        else
            fail "$d/$f differs from the development export"
        fi
    done
    # Matroska writes a random SegmentUID / TrackUID: compare the decoded frames and the byte length
    a=$(ffmpeg -v error -i "$OUT/$d/export10.mkv" -map 0:v -f framemd5 - 2>/dev/null | grep -v '^#')
    b=$(ffmpeg -v error -i "$OUT/st_dev/export10.mkv" -map 0:v -f framemd5 - 2>/dev/null | grep -v '^#')
    if [ -n "$a" ] && [ "$a" = "$b" ] && [ "$(stat -f %z "$OUT/$d/export10.mkv")" = "$(stat -f %z "$OUT/st_dev/export10.mkv")" ]; then
        pass "$d/export10.mkv: decoded frames == development export ($(echo "$a" | wc -l | tr -d ' ') frames), same length"
    else
        fail "$d/export10.mkv differs from the development export"
    fi
done

# 7. the launcher's ffmpeg search and its dialog (development Python, offscreen)
if "$PYTHON" "$HERE/test_launcher.py" "$OUT/launcher" > "$OUT/launcher.txt" 2>&1; then
    pass "launcher: ffmpeg search order and the not-found dialog ($(tail -1 "$OUT/launcher.txt"))"
else
    fail "launcher: $(grep FAIL "$OUT/launcher.txt")"
fi

# 8. nothing was written into the bundle
snapshot > "$OUT/bundle_after.sha256"
if cmp -s "$OUT/bundle_before.sha256" "$OUT/bundle_after.sha256"; then
    pass "bundle unchanged ($(wc -l < "$OUT/bundle_after.sha256" | tr -d ' ') files hashed before and after)"
else
    fail "bundle changed: $(diff "$OUT/bundle_before.sha256" "$OUT/bundle_after.sha256" | head -5)"
fi
echo "       size $(du -sh "$APP" | cut -f1)"
echo "$FAILS failure(s)"
exit $((FAILS > 0))
