# PyInstaller spec of MeatVideo.app (N2b). Build with packaging/build_app.sh (README.md).
# onedir .app for arm64: the repro modules (traced from the launcher) and the AFT port go into the PYZ,
# native/libnvcore.dylib into Contents/Frameworks (= sys._MEIPASS, where repro/nv_native.py looks first),
# data/ref_floor.npy into Contents/Resources/data (linked from Frameworks/data; packaging/main.py points
# nv_profile.reference_floor at it). ffmpeg is not bundled (GPL).
import os

from PyInstaller.utils.hooks import collect_submodules

ROOT = os.path.abspath(os.path.join(SPECPATH, ".."))
REPRO = os.path.join(ROOT, "python")
AFT_SRC = os.path.join(ROOT, "reference/genuine", "analysis", "noise", "aft_port", "src")
LIB = os.path.join(ROOT, "native", "libnvcore.dylib")
REF_FLOOR = os.path.join(ROOT, "data", "ref_floor.npy")
for p in (LIB, REF_FLOOR, os.path.join(AFT_SRC, "nv_aft.py")):
    if not os.path.isfile(p):
        raise SystemExit(f"missing {p} (make -C native lib builds the library)")

# repro/*.py except the scripts the app cannot run: the tkinter UI and the v1..v4 / validation scripts
LEGACY = {"nv_ui", "denoise_video", "denoise_video_v2", "neat_repro", "validate_v2", "validate_v4", "validate_v5"}
repro_modules = sorted(f[:-3] for f in os.listdir(REPRO) if f.endswith(".py") and f[:-3] not in LEGACY)

# packages the app never imports that the graph reaches through optional imports (scipy's array-API
# helpers name dask / jax / torch / cupy, scipy.stats names matplotlib, ...; anaconda has them installed)
EXCLUDES = ["tkinter", "_tkinter", "matplotlib", "IPython", "PyQt5", "PyQt6", "PySide2", "pandas", "PIL",
            "sklearn", "numba", "llvmlite", "torch", "tensorflow", "jax", "jaxlib", "cupy", "cupyx", "dask",
            "distributed", "sparse", "ndonnx", "array_api_strict", "sympy", "notebook", "jupyter", "pyarrow",
            "h5py", "xarray", "bokeh", "panel", "holoviews", "plotly", "skimage", "astropy", "statsmodels",
            "pytest", "_pytest", "pydantic", "huggingface_hub", "fsspec", "cryptography", "lxml",
            # scipy._lib._docscrape tries sphinx, scipy._lib._testutils psutil, numpy.__config__ yaml
            "sphinx", "docutils", "babel", "jinja2", "pygments", "alabaster", "requests", "urllib3", "certifi",
            "zstandard", "psutil", "yaml", "setuptools", "pkg_resources"]
# Qt plugins the UI does not use that pull in QtQuick / QtQml / QtPdf (the virtual keyboard, PDF images)
QT_DROP = ("libqtvirtualkeyboardplugin", "libqpdf", "QtQuick", "QtQml", "QtPdf", "QtVirtualKeyboard")

a = Analysis(
    [os.path.join(SPECPATH, "main.py")],
    pathex=[REPRO, AFT_SRC],
    binaries=[(LIB, ".")],
    datas=[(REF_FLOOR, "data")],
    hiddenimports=repro_modules + ["nv_aft"] + collect_submodules("nv_ui_qt") + ["PySide6.QtTest"],
    hookspath=[],
    runtime_hooks=[],
    excludes=EXCLUDES,
    noarchive=False,
    optimize=0,
)
a.binaries = [b for b in a.binaries if not any(k in b[0] for k in QT_DROP)]
a.datas = [d for d in a.datas if not any(k in d[0] for k in QT_DROP)]
pyz = PYZ(a.pure)
exe = EXE(
    pyz,
    a.scripts,
    [],
    exclude_binaries=True,
    name="MeatVideo",
    debug=False,
    strip=False,
    upx=False,
    console=False,
    argv_emulation=False,
    target_arch="arm64",
    codesign_identity=None,
    entitlements_file=None,
)
coll = COLLECT(exe, a.binaries, a.datas, strip=False, upx=False, name="MeatVideo")
app = BUNDLE(
    coll,
    name="MeatVideo.app",
    icon=None,
    bundle_identifier="local.nvrepro.MeatVideo",
    version="0.2.0",
    info_plist={
        "CFBundleName": "MeatVideo",
        "CFBundleDisplayName": "MeatVideo",
        "NSHighResolutionCapable": True,
        "LSMinimumSystemVersion": "26.0",     # the PySide6 6.12 wheels (and libnvcore) are built for 26.0
        "LSArchitecturePriority": ["arm64"],
    },
)
