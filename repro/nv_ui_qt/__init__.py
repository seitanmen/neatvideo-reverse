"""PySide6 UI for the reproduced Neat Video v6 pipeline (v5 backend: nvfilter.py, AFT: nv_aft_v5.py).

Run from repro/:  python3 -m nv_ui_qt.main [video]
Layout follows ui_spec/UI_SPEC.md (Prepare Profile / Filter Settings pages, Profile / Filter / Tools
menus). Controls whose genuine function has no counterpart in the v5 backend are shown disabled
(see state.PARAM_SPECS, `implemented=False`).
"""
import os
import sys

_REPRO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
if _REPRO not in sys.path:
    sys.path.insert(0, _REPRO)
