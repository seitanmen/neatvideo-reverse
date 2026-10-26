import importlib.util, os, sys
sys.path.insert(0, os.getcwd())
import nv_ui_qt
import importlib.machinery; spec = importlib.util.spec_from_file_location("nv_ui_qt.menus", "/Users/seitan/Desktop/work/reverse/sandbox/omo/coord/backup/menus.py.20261026_pre_lowfix", loader=importlib.machinery.SourceFileLoader("nv_ui_qt.menus", "/Users/seitan/Desktop/work/reverse/sandbox/omo/coord/backup/menus.py.20261026_pre_lowfix"))
m = importlib.util.module_from_spec(spec); sys.modules["nv_ui_qt.menus"] = m; spec.loader.exec_module(m)
sys.argv = [sys.argv[0], "--child"] + sys.argv[1:]
sys.path.insert(0, "/Users/seitan/Desktop/work/reverse/sandbox/omo/coord/genuine_ui/verify")
import v_ofx
if __name__ == "__main__":
    sys.exit(v_ofx.child())
