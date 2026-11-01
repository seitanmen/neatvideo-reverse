#!/bin/bash
# build_ofx.sh - build MeatVideo.ofx (N4a / N4b / N4d) and assemble the OFX bundle:
#
#   MeatVideo.ofx.bundle/Contents/
#     Info.plist                      (com.meatvideo.MeatVideo-OFX; layout of the genuine NeatVideo6.ofx.bundle)
#     MacOS/MeatVideo.ofx             (exports OfxGetPlugin, OfxGetNumberOfPlugins, FoundryOfxAllowPluginUnload only)
#     Frameworks/libnvcore.dylib      (native/, found through the rpath @loader_path/../Frameworks)
#     Helpers/MeatVideoQt.app         (the window of the two buttons, MeatVideoQt --ofx-session (ofx/src/session.h);
#                                      Qt, its cocoa / offscreen platform plug-ins and libnvcore inside: self-contained)
#     Resources/THIRD_PARTY.txt       (OpenFX BSD-3-Clause notice, LZMA SDK note, Qt note)
#
# Usage: ofx/build_ofx.sh [OUTDIR]   (default ofx/build; the bundle is OUTDIR/MeatVideo.ofx.bundle)
# Qt: $QT_PREFIX (default ~/Qt/6.12.0/macos).  Every Mach-O file and the bundle are signed ad hoc (codesign -s -).
# Nothing is installed: copying the bundle to /Library/OFX/Plugins/ is left to the user.
set -euo pipefail

ROOT=$(cd "$(dirname "$0")/.." && pwd)
OUT=${1:-$ROOT/ofx/build}
QT=${QT_PREFIX:-$HOME/Qt/6.12.0/macos}
VERSION=1.0.0

make -C "$ROOT/native" lib
cmake -S "$ROOT/ofx" -B "$OUT" -DCMAKE_BUILD_TYPE=Release >/dev/null
cmake --build "$OUT" -j 2
# the helper: qtui's MeatVideoQt (Release), in a build tree of its own
cmake -S "$ROOT/qtui" -B "$OUT/qtui" -DCMAKE_PREFIX_PATH="$QT" -DCMAKE_BUILD_TYPE=Release -DNV_EMBED_SOURCE_ROOT=OFF >/dev/null
cmake --build "$OUT/qtui" --target MeatVideoQt -j 2

B="$OUT/MeatVideo.ofx.bundle"
rm -rf "$B"
mkdir -p "$B/Contents/MacOS" "$B/Contents/Frameworks" "$B/Contents/Resources" "$B/Contents/Helpers"
cp "$OUT/MeatVideo.ofx" "$B/Contents/MacOS/MeatVideo.ofx"
cp "$ROOT/native/libnvcore.dylib" "$B/Contents/Frameworks/libnvcore.dylib"

# only the bundle-relative rpath (the build tree's native/ is dropped)
BIN="$B/Contents/MacOS/MeatVideo.ofx"
INT=$(xcrun -f install_name_tool)
for rp in $(otool -l "$BIN" | awk '/cmd LC_RPATH/{f=1} f && /path /{print $2; f=0}'); do
    [ "$rp" = "@loader_path/../Frameworks" ] || "$INT" -delete_rpath "$rp" "$BIN"
done

# the helper app: Qt frameworks and the cocoa platform plug-in by macdeployqt, libnvcore and the offscreen platform
# plug-in (tests, headless use) added; only @executable_path/../Frameworks left as rpath
APP="$B/Contents/Helpers/MeatVideoQt.app"
cp -R "$OUT/qtui/MeatVideoQt.app" "$APP"
HEXE="$APP/Contents/MacOS/MeatVideoQt"
mkdir -p "$APP/Contents/Frameworks"
cp "$ROOT/native/libnvcore.dylib" "$APP/Contents/Frameworks/libnvcore.dylib"
for rp in $(otool -l "$HEXE" | awk '/cmd LC_RPATH/{f=1} f && /path /{print $2; f=0}'); do
    "$INT" -delete_rpath "$rp" "$HEXE"
done
"$INT" -add_rpath "@executable_path/../Frameworks" "$HEXE"
"$QT/bin/macdeployqt" "$APP" -libpath="$QT/lib" >/dev/null
mkdir -p "$APP/Contents/PlugIns/platforms"
cp "$QT/plugins/platforms/libqoffscreen.dylib" "$APP/Contents/PlugIns/platforms/"
"$INT" -add_rpath "@loader_path/../../Frameworks" "$APP/Contents/PlugIns/platforms/libqoffscreen.dylib" 2>/dev/null || true
# plug-ins from the app only (else Qt's compiled-in plug-in path of the build machine comes first, whose plug-ins load
# a second copy of the Qt frameworks)
mkdir -p "$APP/Contents/Resources/data"
printf '[Paths]\nPlugins = PlugIns\n' > "$APP/Contents/Resources/qt.conf"
# the reference floor of the profile model, where nvcore.cpp reference_floor() looks first (without it the helper
# computes the same 51 words, ~0.5 s)
cp "$ROOT/data/ref_floor.npy" "$APP/Contents/Resources/data/ref_floor.npy"
# nothing in the helper may point outside it
if otool -L "$HEXE" "$APP"/Contents/Frameworks/*.dylib $(find "$APP/Contents/PlugIns" -name '*.dylib') |
       grep -E "^\s+/(Users|opt|usr/local)/" >/dev/null; then
    echo "the helper still refers to files outside the bundle:" >&2
    otool -L "$HEXE" | grep -E "^\s+/(Users|opt|usr/local)/" >&2
    exit 1
fi

# no path of this checkout / build tree in any file of the bundle (the helper reads data from its own Resources)
if grep -rlF "$ROOT" "$B" >/dev/null 2>&1 || grep -rlF "$OUT" "$B" >/dev/null 2>&1; then
    echo "the bundle names paths of the build machine:" >&2
    grep -rlF -e "$ROOT" -e "$OUT" "$B" >&2
    exit 1
fi

MINOS=$(otool -l "$BIN" | awk '/LC_BUILD_VERSION/{f=1} f && /minos/{print $2; exit}')
sed -e "s/@VERSION@/$VERSION/g" -e "s/@MINOS@/$MINOS/g" "$ROOT/ofx/Info.plist.in" > "$B/Contents/Info.plist"
plutil -lint "$B/Contents/Info.plist" >/dev/null

{
    echo "Meat Video OpenFX plug-in $VERSION - third-party notices"
    echo
    echo "1. OpenFX headers (ofx/vendor/openfx, compiled into MeatVideo.ofx)"
    echo
    cat "$ROOT/ofx/vendor/openfx/LICENSE"
    echo
    echo "2. LZMA SDK 9.20 (native/lzma, compiled into libnvcore.dylib): public domain (native/lzma/SOURCE.md)."
    echo
    echo "3. Qt $(basename "$(dirname "$QT")") (Contents/Helpers/MeatVideoQt.app/Contents/Frameworks, PlugIns): The Qt Company,"
    echo "   used under the GNU LGPL v3 (dynamically linked frameworks, replaceable); https://www.qt.io/licensing/"
} > "$B/Contents/Resources/THIRD_PARTY.txt"

codesign --force --sign - "$B/Contents/Frameworks/libnvcore.dylib"
codesign --force --deep --sign - "$APP"
codesign --force --sign - "$B"
codesign --verify --strict "$APP"
codesign --verify --strict "$B"

echo "exports:"
nm -gU "$BIN" | awk '{print "  " $3}'
echo "rpaths:"
otool -l "$BIN" | awk '/cmd LC_RPATH/{f=1} f && /path /{print "  " $2; f=0}'
echo "libraries:"
otool -L "$BIN" | tail -n +2
echo "helper: $APP ($(du -sh "$APP" | cut -f1))"
echo "helper rpaths:"
otool -l "$HEXE" | awk '/cmd LC_RPATH/{f=1} f && /path /{print "  " $2; f=0}'
echo "bundle: $B"
