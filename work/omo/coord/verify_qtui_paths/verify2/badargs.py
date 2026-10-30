import subprocess, os, sys
env = dict(os.environ, QT_QPA_PLATFORM="offscreen", NV_UI_QT_SETTINGS="/Users/seitan/Desktop/work/reverse/sandbox/omo/coord/verify_qtui_paths/verify2/args.ini")
if __name__ == "__main__":
    for a in ["0 0", "-4 8", "640 -360", "640 360 0", "640 360 nan", "640 360 24 -1", "7 5", "640 360 24 0 --depth 16"]:
        try:
            r = subprocess.run([sys.executable, "-m", "nv_ui_qt.main", "--ofx"] + a.split(), stdin=subprocess.DEVNULL,
                               capture_output=True, timeout=10, cwd="/Users/seitan/Desktop/work/reverse/repro", env=env)
            print(f"--ofx {a!r}: rc={r.returncode} stdout={len(r.stdout)}B stderr={r.stderr.decode()[-160:]!r}")
        except subprocess.TimeoutExpired:
            print(f"--ofx {a!r}: TIMEOUT 10s (hang)")
