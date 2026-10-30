# G9 (native/verify_x86_bridge.sh): with $NV_X86_CAP set, records the libnvcore.dylib calls of this process
# (native/x86_bridge_cap.py), then runs the sitecustomize this directory shadows (e.g. Anaconda's).
import importlib.machinery
import importlib.util
import os
import sys

_here = os.path.dirname(os.path.abspath(__file__))
if os.environ.get("NV_X86_CAP"):
    sys.path.insert(0, os.path.dirname(_here))
    try:
        import x86_bridge_cap
        x86_bridge_cap.install()
    finally:
        sys.path.pop(0)
_spec = importlib.machinery.PathFinder.find_spec(
    "sitecustomize", [p for p in sys.path if os.path.abspath(p or ".") != _here])
if _spec is not None and _spec.loader is not None:
    _m = importlib.util.module_from_spec(_spec)
    _spec.loader.exec_module(_m)
