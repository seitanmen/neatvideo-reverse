"""Verifier: offscreen UI checks with simulated mouse (QTest) on a 1080p clip of test_v02.
   cd repro && QT_QPA_PLATFORM=offscreen python3 ../sandbox/omo/coord/genuine_ui/verify/v_ui.py CLIP OUTDIR"""
import json, math, os, sys, time
os.environ["QT_QPA_PLATFORM"] = "offscreen"
import numpy as np

R = []
def check(name, ok, detail=""):
    R.append((name, bool(ok))); print(f"[{'PASS' if ok else 'FAIL'}] {name} {detail}", flush=True)

def main(video, outdir):
    os.makedirs(outdir, exist_ok=True)
    os.environ["NV_UI_QT_SETTINGS"] = os.path.join(outdir, "ui.ini")
    if os.path.exists(os.environ["NV_UI_QT_SETTINGS"]):
        os.remove(os.environ["NV_UI_QT_SETTINGS"])
    from PySide6.QtCore import Qt, QPoint, QPointF
    from PySide6.QtTest import QTest
    from PySide6.QtWidgets import QApplication, QMessageBox, QMenu
    app = QApplication([sys.argv[0]])
    import nv_band as nb, nv_flat as nf, nv_curve as nc
    from nv_ui_qt.main_window import MainWindow, settings
    from nv_ui_qt import backend
    from denoise_video_v5 import probe, read_frames
    s = settings(); s.setValue("prefs/crf", 0); s.setValue("prefs/jobs", 3); s.sync()
    win = MainWindow(video); win.resize(1600, 1000); win.show()
    st, vw = win.state, win.viewer
    idle = lambda: not win.runner.running and not win.runner.queued
    def wait(cond, to=900):
        t0 = time.time()
        while not cond():
            app.processEvents(); time.sleep(0.01)
            if time.time() - t0 > to: return False
        app.processEvents(); return True
    ok = wait(lambda: win.frames is not None and idle())
    check("load", ok, f"frames={len(win.frames or [])} profile={st.profile.base is not None and st.profile.base['area']}")
    win.tabs.setCurrentIndex(0); app.processEvents()
    mb = [a.text().replace("&", "") for a in win.menuBar().actions()]
    print("menubar:", mb)
    for a in win.menuBar().actions():
        m = a.menu()
        if m is None: continue
        items = []
        for x in m.actions():
            if x.isSeparator(): items.append("---"); continue
            sc = x.shortcut().toString()
            items.append(f"{x.text().replace('&', '')}{' [' + sc + ']' if sc else ''}{'' if x.isEnabled() else ' (disabled)'}")
        print(f"  menu {a.text().replace('&', '')}: {items}")
    def bar(i):
        lay = win.tb_stack.widget(i).layout()
        out = []
        for j in range(lay.count()):
            w = lay.itemAt(j).widget()
            if w is None: out.append("<stretch>"); continue
            out.append(f"{w.text()}{'' if w.isVisible() else '(hidden)'}{'' if w.isEnabled() else '(disabled)'}")
        return out
    print("prepare toolbar:", bar(0)); print("adjust toolbar:", bar(1))
    # viewer-bar mode menu
    mm = win.viewer_bar.mode.menu()
    print("mode menu:", [("§" + x.text()) if x.isSeparator() else x.text() for x in mm.actions()])
    print("zoom text:", win.viewer_bar.zoom.text(), "frame:", win.viewer_bar.size.text())

    print("prepare default mode", vw.mode(), "emphasis", vw.emphasis)
    H, W = win.frames[0][0].shape
    vw.set_zoom(25); app.processEvents()
    pr = vw.panes()[0][1]
    def wpt(x, y):
        p = vw._to_widget(pr, x, y, "rgb"); return QPoint(round(p.x()), round(p.y()))
    def drag(p0, p1, steps=6):
        QTest.mousePress(vw, Qt.LeftButton, Qt.NoModifier, p0); app.processEvents()
        for k in range(1, steps + 1):
            QTest.mouseMove(vw, p0 + (p1 - p0) * k / steps); app.processEvents()
        QTest.mouseRelease(vw, Qt.LeftButton, Qt.NoModifier, p1); app.processEvents()
    win._viewer_click()
    check("no selection -> 'Auto Profile'", win.auto_btn.text() == "Auto Profile", win.auto_btn.text())
    # a) forward drag
    drag(wpt(300, 200), wpt(700, 320))
    a = win.sel_area
    check("forward drag: corner = press point", a is not None and abs(a[1] - 300) <= 4 and abs(a[0] - 200) <= 4
          and abs(a[3] - 400) <= 8 and abs(a[2] - 120) <= 8 and win.auto_btn.text() == "Build Profile", f"{a}")
    print("  bar W/H:", win.viewer_bar.wh.text())
    # b) backward drag
    drag(wpt(900, 600), wpt(500, 400))
    b = win.sel_area
    check("backward drag: press point = lower-right corner", b is not None and abs(b[1] + b[3] - 900) <= 4
          and abs(b[0] + b[2] - 600) <= 4, f"{b}")
    # c) 512 limit forward, d) backward
    drag(wpt(100, 100), wpt(1300, 1000))
    c = win.sel_area
    check("forward limit 512", c is not None and c[2] == 512 and c[3] == 512 and abs(c[1] - 100) <= 4, f"{c}")
    drag(wpt(1800, 1000), wpt(100, 50))
    d = win.sel_area
    check("backward limit 512", d is not None and d[2] == 512 and d[3] == 512 and abs(d[1] + 512 - 1800) <= 4, f"{d}")
    # e) clamp inside image
    drag(wpt(1700, 900), wpt(2600, 1500))
    e = win.sel_area
    check("clamped inside the image", e is not None and e[1] + e[3] <= W and e[0] + e[2] <= H and e[3] >= 210
          and e[2] >= 170, f"{e} W={W} H={H}")
    # f) move by dragging inside
    drag(wpt(300, 200), wpt(700, 320))
    a = win.sel_area
    drag(wpt(a[1] + a[3] // 2, a[0] + a[2] // 2), wpt(a[1] + a[3] // 2 + 100, a[0] + a[2] // 2 + 40))
    f = win.sel_area
    check("move inside keeps size, shifts", f is not None and (f[2], f[3]) == (a[2], a[3])
          and abs(f[1] - a[1] - 100) <= 4 and abs(f[0] - a[0] - 40) <= 4, f"{a} -> {f}")
    drag(wpt(f[1] + 10, f[0] + 10), wpt(f[1] + 3000, f[0] + 3000))
    g = win.sel_area
    check("move clamps at the image edge", g is not None and (g[2], g[3]) == (a[2], a[3]) and g[1] + g[3] == W
          and g[0] + g[2] == H, f"{g}")
    # g) click clears (inside and outside)
    QTest.mouseClick(vw, Qt.LeftButton, Qt.NoModifier, wpt(g[1] + 20, g[0] + 20)); app.processEvents()
    c1 = win.sel_area is None and win.auto_btn.text() == "Auto Profile"
    drag(wpt(300, 200), wpt(700, 320))
    QTest.mouseClick(vw, Qt.LeftButton, Qt.NoModifier, wpt(1500, 900)); app.processEvents()
    c2 = win.sel_area is None
    check("click (inside / outside) clears the selection", c1 and c2, f"{c1} {c2}")

    # colours, NL label, Not Uniform
    from nv_ui_qt.widgets.viewer import sel_colors
    t = win.timeline.frame()
    rgb = win.frame_rgb(t)
    def nl_ind(rect):
        y, x, h, w = rect
        crop = rgb[y:y + h, x:x + w]
        tot = 0.0
        for cch in range(3):
            m = nb.model_levels(np.ascontiguousarray(crop[..., cch], np.float32))[:5]
            tot += float(np.sum(np.asarray(m, np.float64) ** 2))
        return math.sqrt(tot)
    def nu_ind(rect):
        y, x, h, w = rect
        crop = rgb[y:y + h, x:x + w].astype(np.float32)
        Rr, Gg, Bb = (np.ascontiguousarray(crop[..., k]) for k in range(3))
        Y = (0.2989 * Rr + 0.5866 * Gg + 0.1145 * Bb).astype(np.float32)
        planes = (Y, ((Rr - Y) * np.float32(0.71316504)).astype(np.float32), ((Bb - Y) * np.float32(0.56465274)).astype(np.float32))
        bad = [n for n, p, l in zip(("Y", "Cr", "Cb"), planes, (1, 0, 0)) if nf.flatness(nb.band_levels(p), bool(l)) >= 2.0]
        clip = [n for n, p in zip("RGB", (Rr, Gg, Bb)) if nc.is_clip_genuine(p) > 0.33]
        return bad, clip
    vw.set_zoom(100); app.processEvents()
    for rect in ((40, 1000, 15, 200), (40, 1000, 16, 16), (40, 1000, 127, 300), (40, 1000, 128, 128),
                 (0, 0, 216, 512), (500, 600, 300, 300), (100, 1500, 64, 40)):
        win.on_area(*rect); app.processEvents()
        lab = vw.sel_label
        nl = nl_ind(rect)
        exp_lab = "" if min(rect[2], rect[3]) < 16 else f"Noise Level: {nl:.1f}"
        bad, clip = nu_ind(rect)
        exp_warn = [] if min(rect[2], rect[3]) < 16 else (([f"Not Uniform: {', '.join(bad)}"] if bad else []) +
                                                         ([f"Clipping: {', '.join(clip)}"] if clip else []))
        inner, outer = sel_colors(rect[3], rect[2])
        m = min(rect[2], rect[3])
        exp_c = (255, 15, 15) if m <= 15 else (255, 208, 15) if m <= 127 else (15, 192, 15)
        # pixels of the inner colour around the drawn frame
        vw.center = (rect[1] + rect[3] / 2, rect[0] + rect[2] / 2); vw.update(); app.processEvents()
        img = vw.grab().toImage()
        pr0 = vw.panes()[0][1]
        r = vw._wrect(pr0, rect)
        cnt = 0
        for yy in range(int(r.top()) - 12, int(r.bottom()) + 12, 2):
            for xx in (int(r.left()) - 6, int(r.left()) - 3, int(r.right()) + 3, int(r.right()) + 6):
                if 0 <= xx < img.width() and 0 <= yy < img.height():
                    q = img.pixelColor(xx, yy)
                    cnt += (q.red(), q.green(), q.blue()) == exp_c
        check(f"rect {rect}: colour {inner} (exp {exp_c}, drawn px {cnt}), label {lab!r}, warn {vw.sel_warn}",
              inner == exp_c and cnt > 0 and lab == exp_lab and vw.sel_warn == exp_warn
              and win.profile_page.panel.sel_val.text() == ("-" if not exp_lab else f"{nl:.1f}"),
              f"indep NL {nl:.4f} exp_warn {exp_warn} panel {win.profile_page.panel.sel_val.text()}")
    vw.center = None
    # dashed for <= 15? (read the code path) -> visual sample saved
    win.on_area(40, 1000, 15, 200); vw.center = (1100, 48); vw.update(); app.processEvents()
    vw.grab().save(os.path.join(outdir, "sel_red.png"))
    # triangle menu
    win.on_area(300, 400, 200, 300); app.processEvents()
    vw.center = (550, 400); vw.update(); app.processEvents()
    tr = vw.triangle_rect()
    got = []
    def grab_menu():
        w = QApplication.activePopupWidget()
        if isinstance(w, QMenu):
            got.extend(x.text() for x in w.actions()); w.close()
    from PySide6.QtCore import QTimer
    QTimer.singleShot(200, grab_menu)
    QTest.mouseClick(vw, Qt.LeftButton, Qt.NoModifier, tr.center().toPoint())
    t0 = time.time()
    while time.time() - t0 < 1.0:
        app.processEvents(); time.sleep(0.01)
    check("triangle click opens Build Profile (Most Noisy Frame) / (Current Frame); selection kept",
          got == ["Build Profile (Most Noisy Frame)", "Build Profile (Current Frame)"] and win.sel_area == (300, 400, 200, 300),
          f"{got} {win.sel_area}")
    vw.grab().save(os.path.join(outdir, "sel_triangle.png"))
    # < 32: disabled + error
    win.on_area(10, 10, 18, 30); app.processEvents()
    dis = not win.auto_btn.isEnabled() and win.auto_btn.text() == "Build Profile"
    box = win.build_profile(True)
    err = box is not None and "at least 32x32" in box.text()
    if box: box.close()
    check("< 32: button disabled, error box from the menu path", dis and err, f"{dis} {err}")
    # 32..127 confirm, Cancel then Use
    base0 = st.profile.base
    win.on_area(800, 1200, 40, 100); app.processEvents()
    box = win.build_profile(False)
    texts = [b.text() for b in box.buttons()] if box else []
    [b for b in box.buttons() if b.text() == "Cancel"][0].click(); app.processEvents()
    time.sleep(0.3); app.processEvents()
    cancel_ok = idle() and st.profile.base is base0
    box = win.build_profile(False)
    [b for b in box.buttons() if b.text() == "Use the small area"][0].click()
    used = wait(lambda: idle() and st.profile.base is not None and st.profile.base is not base0)
    check("32..127: confirm (Use the small area / Cancel); Cancel builds nothing; Use builds the rect",
          sorted(texts) == ["Cancel", "Use the small area"] and cancel_ok and used
          and st.profile.base["area"] == [800, 1200, 40, 100], f"{texts} {cancel_ok} {st.profile.base and st.profile.base['area']}")
    # 216 x 512 by mouse, Build Profile by the toolbar button
    vw.set_zoom(25); vw.center = None; app.processEvents(); pr = vw.panes()[0][1]
    win._viewer_click()
    drag(wpt(300, 100), wpt(812, 316))
    sa = win.sel_area
    print("  216x512 drag ->", sa, win.viewer_bar.wh.text())
    if sa != (100, 300, 216, 512):
        win.on_area(100, 300, 216, 512)
    t0 = time.time()
    win.auto_btn.click()
    okp = wait(lambda: idle() and st.profile.base is not None and st.profile.base["area"] == [100, 300, 216, 512]
               and st.profile.base.get("aft_applied"))
    print(f"  profile+AFT {time.time() - t0:.1f} s")
    pp = win.profile_page.panel
    check("216x512 profile built (with AFT); Main Sample NL = indep", okp and pp.main_val.text() == f"{nl_ind((100, 300, 216, 512)):.1f}"
          and vw.main_sample == (100, 300, 216, 512),
          f"{st.profile.base['area']} main {pp.main_val.text()} indep {nl_ind((100, 300, 216, 512)):.2f} name {pp.name.currentText()!r}")
    vw.grab().save(os.path.join(outdir, "main_sample.png"))
    win.grab().save(os.path.join(outdir, "win_after_profile.png"))
    # mode switching performance (Prepare page, 1080p) and Adjust defaults
    vw.set_zoom("fit")
    times = {}
    for mode in ("YCrCb +", "RGB", "YCrCb", "Y Freq", "Cr Freq", "Cb Freq", "Normal", "YCrCb +"):
        t0 = time.time(); vw.set_mode(mode); vw.repaint(); okm = wait(vw.ready, 60); vw.repaint()
        times.setdefault(mode, []).append(round(time.time() - t0, 3))
    print("  Prepare mode switch s:", times)
    win.tabs.setCurrentIndex(1); app.processEvents()
    adj_mode, adj_emph = vw.mode(), vw.emphasis
    t0 = time.time(); vw.set_mode("YCrCb +"); vw.repaint(); wait(vw.ready, 60); dt = time.time() - t0
    keys = [k for k in vw._freq if k[1] == "Y+"]
    check("Adjust: default Normal, YCrCb+ without emphasis", adj_mode == "Normal" and not adj_emph
          and any(k[2] is False for k in keys), f"{adj_mode} {adj_emph} keys {keys} ({dt:.2f} s)")
    vw.grab().save(os.path.join(outdir, "adjust_yccplus.png"))
    vw.set_mode("Normal")
    win.tabs.setCurrentIndex(0); app.processEvents()
    check("Prepare keeps its mode", vw.mode() == "YCrCb +" and vw.emphasis, vw.mode())
    win.tabs.setCurrentIndex(1); app.processEvents()
    # export CRF 0 == backend.process_frame
    dst = os.path.join(outdir, "export.mp4")
    lo, n = 10, 3
    win.process_video(dst, start=lo, frames=n)
    okx = wait(lambda: win.export is None, 1800)
    prof = json.load(open(os.path.splitext(dst)[0] + ".ui_profile.json"))
    info = probe(dst)
    gotf = [fr for _, fr in read_frames(dst, info["W"], info["H"], fps=info["fps"])]
    sinfo = probe(video)
    P = st.backend(); Rr = int(P["radius"])
    src = {i: tuple(p.astype(np.float32) for p in fr) for i, fr in read_frames(video, sinfo["W"], sinfo["H"], lo - Rr, lo + n + Rr, sinfo["fps"])}
    exact = len(gotf) == n
    for k in range(min(n, len(gotf))):
        cfr = lo + k
        ids = [i for i in sorted(src) if abs(i - cfr) <= Rr]
        out = backend.process_frame([src[i] for i in ids], ids.index(cfr), prof, P)
        exp = [np.clip(np.rint(o), 0, 255).astype(np.uint8) for o in out]
        exact &= all(np.array_equal(x, y) for x, y in zip(exp, gotf[k]))
    check("export CRF 0 of the 216x512 profile == backend.process_frame", okx and getattr(win, "export_ok", False)
          and prof["area"] == [100, 300, 216, 512] and exact, f"{len(gotf)} frames area {prof['area']}")
    win.close(); app.processEvents()
    nf_ = sum(1 for _, o in R if not o)
    print(f"{len(R) - nf_}/{len(R)} passed")

if __name__ == "__main__":
    main(sys.argv[1], sys.argv[2])
