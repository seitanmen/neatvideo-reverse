#!/bin/bash
# Builds packaging/dist/NeatVideoRepro.app (README.md). Usage: packaging/build_app.sh
#   PYTHON=<python3 with numpy 2.1.3 / scipy 1.15.3 / PySide6 6.12> (default: python3 on PATH)
# The venv (packaging/.venv) shares the base site-packages (--system-site-packages) and adds only
# PyInstaller, so the bundled numpy / scipy are the ones the bit-exact checks were made with.
set -euo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(dirname "$HERE")"
VENV="$HERE/.venv"
PYI_VERSION=6.22.3
PYTHON="${PYTHON:-python3}"

[ "$(uname -m)" = arm64 ] || { echo "error: build on arm64 (the bundle is arm64 only)" >&2; exit 1; }
if [ ! -x "$VENV/bin/python" ]; then
    "$PYTHON" -m venv --system-site-packages "$VENV"
fi
"$VENV/bin/python" -c "import PyInstaller" 2>/dev/null ||
    "$VENV/bin/python" -m pip install --disable-pip-version-check "pyinstaller==$PYI_VERSION"

# the versions the bit-exact checks (native/MIGRATION.md) assume
"$VENV/bin/python" - <<'EOF'
import sys, numpy, scipy, PySide6
want = {"python": "3.13", "numpy": "2.1.3", "scipy": "1.15.3", "PySide6": "6.12"}
have = {"python": "%d.%d" % sys.version_info[:2], "numpy": numpy.__version__, "scipy": scipy.__version__,
        "PySide6": PySide6.__version__}
print("build environment:", have)
bad = [k for k in want if not have[k].startswith(want[k])]
if bad:
    print("warning: differs from the verified environment:", {k: want[k] for k in bad}, file=sys.stderr)
EOF

make -C "$ROOT/native" lib
# PyInstaller runs on the base interpreter with the venv's site-packages (PyInstaller only) in front:
# inside a venv on top of conda, PyInstaller's numpy hook cannot use its conda support (sys.prefix would
# be the venv), and nothing is installed into the base environment either way
BASE_PY="$("$VENV/bin/python" -c 'import sys; print(sys._base_executable)')"
VENV_SITE="$("$VENV/bin/python" -c 'import sysconfig; print(sysconfig.get_paths()["purelib"])')"
PYTHONPATH="$VENV_SITE" "$BASE_PY" "$HERE/pyi_build.py" --noconfirm --clean --log-level WARN \
    --distpath "$HERE/dist" --workpath "$HERE/build" "$HERE/nvrepro.spec"

APP="$HERE/dist/NeatVideoRepro.app"
codesign --force --deep -s - "$APP"
codesign --verify --deep --strict --verbose=2 "$APP"
du -sh "$APP"
echo "built $APP"
