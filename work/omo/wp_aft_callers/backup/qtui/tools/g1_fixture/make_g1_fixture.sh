#!/bin/sh
# Regenerates qtui/tests/fixtures/g1_before_combine.json with the code BEFORE G1 (G1 review L-1): the expected
# Y / Cb / Cr curves, sigma and processed-frame hashes of qtui_selftest g1EditMatchesAftCombine, computed by the
# Y / Cb / Cr combination of the base commit (nvc_aft_combine of 5dabc1f), not by the code under test.
#
#   sh qtui/tools/g1_fixture/make_g1_fixture.sh [BASE_COMMIT] [WORK_DIR]
#
# Steps: git archive BASE_COMMIT (native, qtui, data) into WORK_DIR, add g1_fixture.cpp as a qtui target, build
# libnvcore + the tool there, run it on test.mp4 frame 4 (the frame the self-test uses) and write the fixture.
set -eu
ROOT=$(cd "$(dirname "$0")/../../.." && pwd)
BASE=${1:-5dabc1f}
WORK=${2:-${TMPDIR:-/tmp}/g1_fixture_build}
QT=${QT_PREFIX:-$HOME/Qt/6.12.0/macos}
rm -rf "$WORK"
mkdir -p "$WORK"
git -C "$ROOT" archive "$BASE" native qtui data | tar -x -C "$WORK"
cp "$ROOT/qtui/tools/g1_fixture/g1_fixture.cpp" "$WORK/qtui/tests/g1_fixture.cpp"
cat >> "$WORK/qtui/CMakeLists.txt" <<'CM'

add_executable(g1_fixture tests/g1_fixture.cpp)
target_link_libraries(g1_fixture PRIVATE nvui)
set_target_properties(g1_fixture PROPERTIES BUILD_RPATH "${NVCORE_DIR}")
CM
make -C "$WORK/native" lib
cmake -S "$WORK/qtui" -B "$WORK/qtui/build" -DCMAKE_PREFIX_PATH="$QT"
cmake --build "$WORK/qtui/build" -j 4 --target g1_fixture
"$WORK/qtui/build/g1_fixture" "$ROOT/test.mp4" 4 "$ROOT/qtui/tests/fixtures/g1_before_combine.json"
echo "wrote $ROOT/qtui/tests/fixtures/g1_before_combine.json (from $BASE)"
