import importlib.util, os, sys
WT = "/Users/seitan/Desktop/work/reverse/.claude/worktrees/n4-ofx"
B = os.environ["VN4_BUILD"]
spec = importlib.util.spec_from_file_location("verify_ofx", WT + "/ofx/tools/verify_ofx.py")
m = importlib.util.module_from_spec(spec); spec.loader.exec_module(m)
m.BUILD, m.MINIHOST, m.BUNDLE, m.FSGUARD = B, B + "/minihost", B + "/MeatVideo.ofx.bundle", B + "/libfsguard.dylib"
sys.argv = [WT + "/ofx/tools/verify_ofx.py"] + sys.argv[1:]
sys.exit(m.main())
