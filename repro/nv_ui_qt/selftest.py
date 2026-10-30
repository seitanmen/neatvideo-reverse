"""Off-screen self-test of the Qt UI (no window is shown).

    cd repro && QT_QPA_PLATFORM=offscreen python3 -m nv_ui_qt.selftest VIDEO OUTDIR

Writes only into OUTDIR (settings, presets, profiles, a short lossless export). Checks: load +
preview, Auto Profile (current / most noisy frame), Build Profile, slider -> preview update,
per-channel band settings, undo/redo, AFT, Manual Fine-Tune + Auto Complete, preset / profile
save + load, batch profiler, and that the export equals nv.process_frame_v5 bit for bit (CRF 0).
Layout checks: panel order (Temporal / Spatial / General, Luminance -> Cr -> Cb, High -> Ultra Low),
common Frequencies rows, Mix with Original Enable, profile Noise Level, hold-for-original, viewer
panes and splits, thumbnails, Beginner / Advanced Mode, Auto / Build Profile button.
Genuine UI (2026-10-26, sandbox/omo/coord/genuine_ui/SPEC.md): display formulas (display.py), free
rectangular selection by mouse (corner = press point, <= 512 px, move inside, click clears), frame colours,
Noise Level label = display.noise_level, Not Uniform, Build Profile size rules (< 32 error, 32..127
confirmation), menus / toolbars in the genuine order, viewer mode per page, rectangular profile through
the export.
Preferences > ffmpeg (G2, ffmpeg_path.py): a path without ffmpeg / ffprobe warns and keeps the dialog, the
binary is saved and used, a video opens.
High bit depth (written into OUTDIR as src10.mkv): Preferences > Bit depth 10 reloads a 10-bit copy,
frames hold value/4 exactly, status bar, preview, FFV1 export == backend.process_frame bit for bit.
Profile Check and the panel (2026-10-26, sandbox/omo/coord/genuine_ui2/SPEC.md R1-R3): Begin / toolbar /
menu = one Assist Image toggle (image == spatial-only 100% filter of the frame, bottom panes without
emphasis, live update on Noise Level, ends on Finish / Adjust page / Reset Profile), Profile Viewer,
panel without a scroll bar at 1800 x 1125 with Noise Level and Frequencies open, dark info box, equalizer
button order, selection Noise Level "(+n%)" against the Main Sample.
Noise Level Check (2026-10-26, sandbox/omo/coord/verify_quality/nlc/SPEC.md R1-R5): Temporal Noise Level in
the temporal stage only (+0% = before; export with -40% == backend.process_frame), Begin = temporal mask image
(1 on a repeated frame, 0 at -100%), live update, panel / toolbar menu / Tools menu in sync, panel state.
"""
import json
import os
import subprocess
import sys
import time

os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")

import numpy as np
from PySide6.QtCore import Qt
from PySide6.QtTest import QTest
from PySide6.QtWidgets import QApplication

RESULTS = []


def check(name, ok, detail=""):
    RESULTS.append((name, bool(ok)))
    print(f"[{'PASS' if ok else 'FAIL'}] {name} {detail}", flush=True)


def wait(app, cond, timeout=600.0):
    t0 = time.time()
    while not cond():
        app.processEvents()
        time.sleep(0.01)
        if time.time() - t0 > timeout:
            return False
    app.processEvents()
    return True


def genuine_ui_checks(app, win, wait, idle, outdir):
    """Checks of the genuine-UI changes (display formulas, selection, buttons, menus)."""
    from nv_ui_qt import display as D
    from nv_ui_qt.widgets.viewer import PANES, sel_colors
    st, vw = win.state, win.viewer
    # display formulas on a synthetic image (STATIC.md §1)
    rng = np.random.default_rng(1)
    rgb = rng.uniform(0, 255, (40, 64, 3)).astype(np.float32)
    R, G, B = rgb[..., 0], rgb[..., 1], rgb[..., 2]
    Y = 0.2989 * R + 0.5866 * G + 0.1145 * B
    Cr, Cb = (R - Y) * 0.71316504, (B - Y) * 0.56465274
    ident = np.array_equal(D.render_simple("rgb", rgb, 50), np.clip(np.rint(rgb), 0, 255).astype(np.uint8))
    yok = np.abs(D.render_simple("Y", rgb, 50).astype(float) - np.rint(Y)[..., None]).max() <= 1
    m, h = np.array([143.5, 107.5, 112.5]), np.array([112.0, 108.0, 113.0])
    crok = np.abs(D.render_simple("Cr", rgb, 50).astype(float) -
                  np.clip(np.rint(m + h * Cr[..., None] / 64.5), 0, 255)).max() <= 1
    gp = D.render_simple("R", rgb, 50).astype(float)
    rok = np.array_equal(gp[..., 0], np.rint(R)) and np.abs(gp[..., 1] - (R * 201 / 256 - 0.5 + 0.5 * 201 / 256)).max() <= 1
    bright = D.render_simple("rgb", rgb, 100).mean() > D.render_simple("rgb", rgb, 50).mean() > \
        D.render_simple("rgb", rgb, 0).mean()
    pl, lv = D.freq_images(rgb, "Y+", True)
    pf, lf = D.freq_images(rgb, "Y", True)
    fr = len(pl) == 3 and len(pf) == 3 and all(p.dtype == np.int16 and p.shape == (40, 64) for p in pl + pf) \
        and all(a < b for a, b in lv + lf)
    check("display: Normal identity at 50, Y / Cr / R pane formulas, brightness, Enhanced / Freq images",
          ident and yok and crok and rok and bright and fr, f"{ident} {yok} {crok} {rok} {bright} {fr}")
    check("display: frame colours by min(w, h) (<= 15 red, 16..127 yellow, >= 128 green)",
          sel_colors(30, 15)[0] == (255, 15, 15) and sel_colors(512, 16)[0] == (255, 208, 15)
          and sel_colors(200, 128)[0] == (15, 192, 15))

    # menus and toolbars in the genuine order; viewer mode per page
    win._viewer_click()
    mb = [a.text().replace("&", "") for a in win.menuBar().actions()]

    def bar(i):
        lay = win.tb_stack.widget(i).layout()
        return [lay.itemAt(j).widget().text() for j in range(lay.count()) if lay.itemAt(j).widget()]
    tb, ta = bar(0), bar(1)
    check("menus Effect View Profile Filter Variants Tools Help; toolbars in the genuine order",
          mb == ["Effect", "View", "Profile", "Filter", "Variants", "Tools", "Help"]
          and tb == ["Auto Profile", "Generic Profile", "Load Profile", "Profile Check", "Variants"]
          and ta[1:] == ["Preview", "Play All", "Filter Settings Check", "Variants"], f"{mb} {tb} {ta}")
    win.tabs.setCurrentIndex(1)
    vw.set_mode("Normal")
    win.tabs.setCurrentIndex(0)
    win.viewer.set_mode("YCrCb +")
    win.tabs.setCurrentIndex(1)
    adj = vw.mode() == "Normal" and not vw.emphasis and win.first_modes == ["YCrCb +", "Normal"]
    split_off = not win.actions["split_2 x 2"].isEnabled()
    vw.set_mode("RGB")
    win.tabs.setCurrentIndex(0)
    prep = vw.mode() == "YCrCb +" and vw.emphasis
    win.tabs.setCurrentIndex(1)
    back = vw.mode() == "RGB"
    vw.set_mode("Normal")
    check("viewer mode kept per page (defaults Prepare YCrCb+, Adjust Normal), Split disabled in Normal",
          adj and prep and back and split_off, f"{win.first_modes} {adj} {prep} {back} {split_off}")

    # free rectangular selection by mouse on the Prepare page
    win.tabs.setCurrentIndex(0)
    vw.set_mode("YCrCb +")
    vw.set_zoom(100)
    app.processEvents()
    pr = vw.panes()[0][1]
    c = pr.center()
    sx, sy = c.x() - 150, c.y() - 60

    def drag(p0, p1):
        QTest.mousePress(vw, Qt.LeftButton, Qt.NoModifier, p0)
        for k in range(1, 6):
            QTest.mouseMove(vw, p0 + (p1 - p0) * k / 5)
            app.processEvents()
        QTest.mouseRelease(vw, Qt.LeftButton, Qt.NoModifier, p1)
        app.processEvents()

    from PySide6.QtCore import QPoint
    drag(QPoint(int(sx), int(sy)), QPoint(int(sx + 200), int(sy + 50)))
    a = win.sel_area
    rect_ok = a is not None and (a[2], a[3]) == (50, 200) and win.auto_btn.text() == "Build Profile"
    lab = vw.sel_label
    nl = D.noise_level(win.frame_rgb(win.timeline.frame())[a[0]:a[0] + a[2], a[1]:a[1] + a[3]])
    from nv_ui_qt.main_window import area_rect
    base = st.profile.base
    main_nl = None
    if base is not None:                        # the label adds (%+d%) against the Main Sample (R3)
        my, mx, mh, mw = area_rect(base["area"])
        main_nl = D.noise_level(win.frame_rgb(int(base["ui_frame"]))[my:my + mh, mx:mx + mw])
    suffix = f" ({int((nl / main_nl - 1.0) * 100.0):+d}%)" if main_nl else ""
    nl_ok = lab == f"Noise Level: {nl:.1f}{suffix}" and win.profile_page.panel.sel_val.text() == f"{nl:.1f}"
    drag(QPoint(int(sx + 100), int(sy + 25)), QPoint(int(sx + 130), int(sy + 45)))     # inside: move
    b = win.sel_area
    moved = b is not None and (b[2], b[3]) == (a[2], a[3]) and (b[0] - a[0], b[1] - a[1]) == (20, 30)
    QTest.mouseClick(vw, Qt.LeftButton, Qt.NoModifier, QPoint(int(c.x() + 200), int(c.y() + 100)))
    app.processEvents()
    cleared = win.sel_area is None and win.auto_btn.text() == "Auto Profile"
    # each side at most 512 from the press point, inside the frame
    vw.set_zoom(50)
    app.processEvents()
    c = vw.panes()[0][1].center()
    H, W = win.frames[0][0].shape
    p0 = QPoint(int(c.x() + (10 - W / 2) * 0.5), int(c.y() + (10 - H / 2) * 0.5))      # image (10, 10)
    drag(p0, p0 + QPoint(600, 600))             # 1200 image px each way
    big = win.sel_area
    lim = big is not None and big[3] == min(512, W - big[1]) and big[2] == min(512, H - big[0]) and big[1] <= 11
    check("free rectangle by mouse (corner = press point), Noise Level label, move inside, click clears, "
          "<= 512 px", rect_ok and nl_ok and moved and cleared and lim, f"{a} {lab!r} {nl:.2f} {b} {big}")
    vw.set_zoom("fit")

    # Build Profile rules: < 32 px disabled / error; Not Uniform label
    win.on_area(10, 10, 18, 30)
    dis = not win.auto_btn.isEnabled() and win.auto_btn.text() == "Build Profile"
    box = win.build_profile(False)
    err = box is not None and "at least 32x32" in box.text()
    box.close()
    win.on_area(0, 0, 64, 64)
    y0 = win.frames[0][0].shape[0] // 2
    win.on_area(y0 - 100, 0, 200, 200)          # a detailed area of the frame
    bad, clip = D.not_uniform(win.frame_rgb(win.timeline.frame())[y0 - 100:y0 + 100, 0:200])
    warn_ok = vw.sel_warn == ([f"Not Uniform: {', '.join(bad)}"] if bad else []) + \
        ([f"Clipping: {', '.join(clip)}"] if clip else [])
    m = win._sel_menu(vw.mapToGlobal(QPoint(10, 10)))
    menu_ok = m is not None and [x.text() for x in m.actions()] == ["Build Profile (Most Noisy Frame)",
                                                                      "Build Profile (Current Frame)"]
    m.close()
    win._viewer_click()
    check("Build Profile disabled / error under 32 px, Not Uniform label, triangle menu",
          dis and err and warn_ok and menu_ok, f"{dis} {err} {vw.sel_warn} {bad} {clip}")

    # bottom panes: same scale as the primary pane, 2 : 1 rows; screenshots of the modes
    for mode in ("YCrCb +", "YCrCb", "RGB", "Y Freq"):
        vw.set_mode(mode)
        t0 = time.time()
        ok = wait(app, vw.ready, 60)
        vw.grab().save(os.path.join(outdir, f"viewer_{mode.replace(' ', '_').replace('+', 'plus')}.png"))
        print(f"    viewer mode {mode}: panes ready in {time.time() - t0:.2f} s", flush=True)
    names = [n for n, _ in vw.panes()]
    check("component panes computed for every mode", ok and names == ["rgb"] + PANES["Y Freq"])
    vw.set_mode("YCrCb +")
    win.tabs.setCurrentIndex(1)


def profile_check_checks(app, win, wait, idle, outdir):
    """R1-R3 of sandbox/omo/coord/genuine_ui2/SPEC.md (Profile Check = Assist Image mode, the Device Noise
    Profile panel, the selection's Noise Level suffix)."""
    from PySide6.QtGui import QKeySequence
    from PySide6.QtWidgets import QLabel
    from nv_ui_qt import display as D
    from nv_ui_qt.main_window import area_rect, assist_params
    from nv_ui_qt.state import UI_BANDS
    from nv_ui_qt.widgets.profile_panel import EQ_TIPS, ProfilePanel
    from nv_ui_qt.widgets.rows import ACTIVE_TITLE, Header
    st, vw, ppn = win.state, win.viewer, win.profile_page.panel
    win.tabs.setCurrentIndex(0)
    vw.set_mode("YCrCb +")
    wait(app, idle)

    def synced(on):
        return (win.pcheck == on and win.check_btn.isChecked() == on and win.actions["p_check"].isChecked() == on
                and ppn.check.button.isChecked() == on and ppn.check.button.text() == ("Finish" if on else "Begin")
                and (ACTIVE_TITLE in ppn.check.title.text()) == on and (ACTIVE_TITLE in ppn.tuning.title.text()) == on)

    # R1: Begin -> Assist Image of the current frame (spatial only, every amount 100%), in a worker
    ppn.check.button.click()
    on_ok = synced(True)
    got = wait(app, lambda: vw.showing_pcheck() and idle() and vw.ready(), 120)
    t = win.timeline.frame()
    P = assist_params()
    pok = P["temporal"] == 0.0 and all(ch["amount"] == 1.0 and all(v == 1.0 for v in ch["bands"].values())
                                       for ch in P["ui_channels"].values())
    rot = (win.info["rotation"] // 90) % 4
    exp = np.ascontiguousarray(np.rot90(win.engine.assist(t, st.effective_profile(), P).astype(np.float32), rot))
    same = got and np.array_equal(vw._f["pcheck"], exp)
    differs = got and vw._source() == "pcheck" and float(np.abs(vw._f["pcheck"] - vw._f["orig"]).mean()) > 0.1
    keys = [k for k in vw._freq if k[0] == vw._tok["pcheck"]]
    noemph = bool(keys) and all(k[2] is False for k in keys)
    pl, lv = D.freq_images(exp, "Y+", False)
    want = D.render_freq(["Y+"], [pl[0]], [lv[0]], vw.brightness)[0]
    qi = vw.pane_image("Y+", "pcheck")
    bits = np.frombuffer(qi.constBits(), np.uint8).reshape(qi.height(), qi.bytesPerLine())[:, :3 * qi.width()]
    pane_ok = np.array_equal(bits.reshape(qi.height(), qi.width(), 3), want)
    vw.grab().save(os.path.join(outdir, "pcheck_viewer.png"))
    check("R1 Begin: Assist Image (spatial only, 100%) on all panes, bottom Enhanced without emphasis; "
          "panel / toolbar / menu in sync", on_ok and got and pok and same and differs and noemph and pane_ok,
          f"{on_ok} {got} {pok} {same} {differs} {noemph} {pane_ok}")
    # live update on the profile Noise Level (thinned out), the old image comes back from the cache
    before = vw._f["pcheck"].copy()
    st.new_edit()
    ppn.tune["all"].slider.setValue(50)
    upd = wait(app, lambda: idle() and not win.pc_timer.isActive() and win._pc_key() in win._pc_cache
               and not np.array_equal(vw._f["pcheck"], before), 120)
    exp2 = np.rot90(win.engine.assist(t, st.effective_profile(), P).astype(np.float32), rot)
    upd = upd and np.array_equal(vw._f["pcheck"], exp2) and st.effective_profile() is not None
    st.undo.undo()
    app.processEvents()
    cached = wait(app, lambda: idle() and not win.pc_timer.isActive() and np.array_equal(vw._f["pcheck"], before), 30)
    check("R1 Noise Level slider recomputes the Assist Image; undo shows the cached one", upd and cached,
          f"{upd} {cached}")
    # Finish from the toolbar; menu starts it; Adjust page and Reset Profile end it
    win.check_btn.click()
    off_ok = synced(False) and vw._source() == "orig" and not vw.showing_pcheck()
    win.actions["p_check"].trigger()
    m_on = synced(True)
    win.tabs.setCurrentIndex(1)
    page_off = synced(False)
    win.tabs.setCurrentIndex(0)
    ppn.check.button.click()
    win.reset_profile()
    reset_off = synced(False) and not ppn.check.isEnabled() and not win.actions["p_check"].isEnabled() \
        and not win.check_btn.isVisible()
    st.undo.undo()
    back = ppn.check.isEnabled() and win.actions["p_check"].isEnabled() and not win.pcheck
    check("R1 Finish (toolbar) / menu toggle / Adjust page / Reset Profile; disabled without a profile",
          off_ok and m_on and page_off and reset_off and back, f"{off_ok} {m_on} {page_off} {reset_off} {back}")
    # Profile Viewer = the former dialog plus the Repro section (Profile mode kept in the preferences)
    dlg = win.profile_viewer()
    pv = dlg.windowTitle() == "Profile Viewer" and dlg.mode.currentText() == "auto"
    dlg.mode.setCurrentText("spatial")
    pv = pv and win._profile_mode() == "spatial" and win.prefs["profile_mode"] == "spatial"
    dlg.mode.setCurrentText("auto")
    labels = " ".join(w.text() for w in dlg.findChildren(QLabel))
    pv = pv and "Sample area" in labels and "Match" in labels and "Band ratios" in labels
    dlg.close()
    check("R1 Profile Viewer… (Shift+Ctrl+I): repro profile details and Profile mode", pv and
          win.actions["p_viewer"].shortcut() == QKeySequence("Ctrl+Shift+I"))

    # R2: the panel at 1800 x 1125 with Noise Level and Frequencies open: no scroll bar
    size0 = win.size()
    win.resize(1800, 1125)
    ppn.tune_header.set_expanded(True)
    ppn.freq_header.set_expanded(True)
    for _ in range(5):
        app.processEvents()
    sb = ppn.scroll.verticalScrollBar()
    fits = not sb.isVisible() and sb.maximum() == 0
    ppn.grab().save(os.path.join(outdir, "profile_panel_open.png"))
    titles = [h.title_text for h in ppn.findChildren(Header)]
    texts = " ".join(w.text() for w in ppn.findChildren(QLabel))
    gone = "Repro" not in titles and "Quick Start Guide" not in texts and "Currently working" not in texts \
        and "(?)" not in ppn.tuning.title.text()
    im = ppn.info.grab().toImage()
    bg = im.pixelColor(im.width() - 4, im.height() - 4)
    light = max(im.pixelColor(x, y).lightness() for x in range(0, im.width(), 2) for y in range(0, im.height(), 2))
    dark = bg.lightness() < 80 and light > 170
    lay = ppn.curve.buttons
    ws = [lay.itemAt(i).widget() for i in range(lay.count()) if lay.itemAt(i).widget()]
    order = ws == [ppn.b_aft, ppn.b_manual, ppn.b_complete] + \
        [ppn.curve.target_buttons[k] for k in ("all", "Y", "Cr", "Cb")] + [ppn.complete_lbl, ppn.b_reset]
    tips = (ppn.b_aft.toolTip().startswith("Use to automatically fine-tune the current noise profile using the "
                                           "whole input image") and ppn.b_manual.toolTip() == EQ_TIPS["manual"]
            and not ppn.b_manual.isEnabled() and ppn.b_reset.toolTip() == "Reset all fine-tuning"
            and ppn.curve.target_buttons["all"].toolTip().startswith("Show the noise graphs (noise level vs "
                                                                     "brightness level) for all three color channels"))
    samples = all(not ppn.band_samples[b].pixmap().isNull() for b, _ in UI_BANDS)
    fresh = ProfilePanel()
    collapsed = fresh.tune_header.body.isHidden() and fresh.freq_header.body.isHidden()
    fresh.deleteLater()
    win.resize(size0)
    app.processEvents()
    check("R2 panel: no scroll bar at 1800x1125 (Noise Level + Frequencies open), dark info box, no Repro / "
          "guide, equalizer buttons in the genuine order with its tooltips, band samples, collapsed defaults",
          fits and gone and dark and order and tips and samples and collapsed,
          f"{fits} {sb.maximum()} {gone} {dark} ({bg.lightness()}, {light}) {order} {tips} {samples} {collapsed}")

    # R3: Noise Level: x.x (%+d%) against the Main Sample, truncated toward zero
    base = st.profile.base
    mrect = area_rect(base["area"])
    win.timeline.set_frame(int(base["ui_frame"]), emit=True)
    main_nl = D.noise_level(win.frame_rgb(int(base["ui_frame"]))[mrect[0]:mrect[0] + mrect[2],
                                                                   mrect[1]:mrect[1] + mrect[3]])
    win.on_area(*mrect)
    zero = vw.sel_label == f"Noise Level: {main_nl:.1f} (+0%)"
    H, W = win.frames[0][0].shape
    rect = (H // 2, W // 4, 64, 96)
    win.on_area(*rect)
    sel = D.noise_level(win.frame_rgb(win.timeline.frame())[rect[0]:rect[0] + 64, rect[1]:rect[1] + 96])
    r = sel / main_nl - 1.0
    pct = int(r * 100.0)                       # C (int) cast: toward zero
    other = vw.sel_label == f"Noise Level: {sel:.1f} ({'+' if pct >= 0 else '-'}{abs(pct)}%)"
    win.reset_profile()
    plain = vw.sel_label == f"Noise Level: {sel:.1f}"
    st.undo.undo()
    win._viewer_click()
    check("R3 selection Noise Level (%+d%) vs Main Sample (+0% on the sample itself, none without a profile)",
          zero and other and plain, f"{vw.sel_label!r} {main_nl:.3f} {sel:.3f} {zero} {other} {plain}")
    win.tabs.setCurrentIndex(1)


def nlc_checks(app, win, wait, idle, outdir):
    """R1-R5 of sandbox/omo/coord/verify_quality/nlc/SPEC.md (Temporal Noise Level, Noise Level Check =
    temporal filter mask, its panel state and the three entries)."""
    import nvfilter as nv
    from denoise_video_v5 import probe, read_frames
    from nv_ui_qt import backend
    from nv_ui_qt import display as D
    from nv_ui_qt.state import backend_params, default_params
    from nv_ui_qt.widgets.rows import ACTIVE_TITLE
    st, vw, fpn = win.state, win.viewer, win.filter_page.panel
    win.tabs.setCurrentIndex(1)
    wait(app, idle)
    rot = (win.info["rotation"] // 90) % 4

    def synced(on):
        return (win.nlc == on and fpn.nl_check.button.isChecked() == on
                and fpn.nl_check.button.text() == ("Finish" if on else "Begin")
                and (ACTIVE_TITLE in fpn.nl_check.title.text()) == on and win.fsc_nlc.isChecked() == on
                and win.actions["nlc"].isChecked() == on and win.fsc_btn.isChecked() == on
                and win.fsc_btn.text() == ("Noise Level Check" if on else "Filter Settings Check"))

    def expected():
        t, P = win.timeline.frame(), st.backend()
        m = win.engine.mask(t, st.effective_profile(), win.prof_token, P)
        return np.ascontiguousarray(np.rot90(D.nlc_image(win.frame_rgb(t), m), rot)), m

    def shown():
        return wait(app, lambda: idle() and not win.nlc_timer.isActive() and vw.showing_nlc()
                    and win._nlc_shown == win._nlc_key(), 120)

    # (a) Begin (panel) -> mask image on the primary pane, viewer Normal; Finish restores the mode
    vw.set_mode("RGB")
    fpn.nl_check.button.click()
    on_ok = synced(True) and vw.mode() == "Normal"
    got = shown()
    exp, m0 = expected()
    same = got and vw._source() == "nlc" and np.array_equal(vw._f["nlc"], exp)
    comp = vw.composite()
    img_ok = comp.shape[:2] == exp.shape[:2] and not win.msg.text().startswith("error")
    vw.grab().save(os.path.join(outdir, "nlc_viewer.png"))
    fpn.nl_check.button.click()
    off_ok = synced(False) and vw._source() != "nlc" and not vw.showing_nlc() and vw.mode() == "RGB"
    vw.set_mode("Normal")
    check("NLC (a) Begin: temporal mask image (display.nlc_image) on the primary pane, viewer Normal; Finish "
          "restores", on_ok and got and same and img_ok and off_ok,
          f"{on_ok} {got} {same} {img_ok} {off_ok} mean m {float(m0.mean()):.3f}")

    # (b) m = 1 where every neighbour has weight 1 (the same frame five times), 0 at Temporal Noise Level 0%
    prof = st.effective_profile()
    t = win.timeline.frame()
    planes = win.engine.window(t, 0, None)[1][0]
    P = st.backend()
    tmp, rv = backend.temporal([planes] * 5, 2, prof, P)
    m_same = backend.temporal_mask(rv[0], 5, P["temporal"])
    ui0 = dict(st.params, t_noise=0.0)
    m_zero = win.engine.mask(t, prof, win.prof_token, backend_params(ui0))
    edge = (not backend.temporal_mask(rv[0], 1, 1.0).any() and not backend.temporal_mask(rv[0], 5, 0.0).any())
    check("NLC (b) mask = 1 on a repeated frame (all neighbour weights 1), 0 at Temporal Noise Level -100%, "
          "0 for one frame / Amount 0", m_same.min() > 0.999 and m_zero.max() < 1e-3 and edge,
          f"min {m_same.min():.6f} max(0%) {m_zero.max():.2e} {edge}")

    # (b') live recomputation on Noise Level / Radius / Enable, results only for the current key
    fpn.nl_check.button.click()
    shown()
    before = vw._f["nlc"].copy()
    st.new_edit()
    fpn.controls["t_noise"].slider.setValue(-60)                     # display -60% = 40
    nl_ok = shown() and st.params["t_noise"] == 40.0 and not np.array_equal(vw._f["nlc"], before)
    exp, m1 = expected()
    nl_ok = nl_ok and np.array_equal(vw._f["nlc"], exp) and m1.mean() < m0.mean()
    st.set_param("t_radius", 1)
    r_ok = shown() and np.array_equal(vw._f["nlc"], expected()[0])
    st.set_param("t_enable", False)
    e_ok = shown() and np.array_equal(vw._f["nlc"], np.ascontiguousarray(
        np.rot90(D.nlc_image(win.frame_rgb(t), np.zeros(m1.shape, np.float32)), rot)))
    for _ in range(3):
        st.undo.undo()
    undo_ok = shown() and st.params["t_noise"] == 100.0 and np.array_equal(vw._f["nlc"], before)
    nxt = t + 1 if t + 1 < len(win.frames) else t - 1
    win.timeline.set_frame(nxt, emit=True)
    stale_gone = not vw.showing_nlc()                                # no image of the previous frame
    f_ok = shown() and win._nlc_shown[0] == nxt and np.array_equal(vw._f["nlc"], expected()[0])
    win.timeline.set_frame(t, emit=True)
    shown()
    check("NLC (b') Noise Level / Radius / Enable recompute the mask, undo shows the cached one, another frame "
          "drops the old image", nl_ok and r_ok and e_ok and undo_ok and stale_gone and f_ok,
          f"{nl_ok} {r_ok} {e_ok} {undo_ok} {stale_gone} {f_ok} mean -60% {float(m1.mean()):.3f}")

    # (c) t_noise 100% = the temporal stage before the change (nv.temporal_v5 with P unchanged); t_noise != 100%
    # scales the temporal noise level only
    idx, wp = win.engine.window(t, int(P["radius"]), None)
    ci = idx.index(t)
    ok = True
    for ui in (default_params(), dict(default_params(), bamt_Cr_medium=60.0, t_radius=1),
               dict(default_params(), mix_enable=True, mix=25.0, adv_noise_level=150.0)):
        Pb = backend_params(ui)
        ref_t = nv.temporal_v5(wp[max(0, ci - Pb["radius"]):ci + Pb["radius"] + 1], min(ci, Pb["radius"]), prof, Pb)
        a = backend.process_frame(wp, ci, prof, Pb)
        b = backend.finish(wp, ci, *ref_t, prof, Pb)
        ok &= all(np.array_equal(x, y) for x, y in zip(a, b))
    Pn = backend_params(dict(default_params(), t_noise=60.0))
    ref_t = nv.temporal_v5(wp, ci, prof, dict(Pn, noise_level=Pn["noise_level"] * 0.6))
    a = backend.process_frame(wp, ci, prof, Pn)
    scaled = all(np.array_equal(x, y) for x, y in zip(a, backend.finish(wp, ci, *ref_t, prof, Pn)))
    eng = win.engine.process(t, prof, win.prof_token, backend_params(default_params()))[0]
    prev = np.array_equal(eng, win.engine.to_rgb([np.clip(o, 0, 255) for o in backend.process_frame(
        wp, ci, prof, backend_params(default_params()))]))
    check("NLC (c) Temporal Noise Level +0% = the temporal stage before the change (process_frame, preview); "
          "other values scale only the temporal noise level", ok and scaled and prev, f"{ok} {scaled} {prev}")

    # (c) export with Temporal Noise Level -40% (CRF 0) == backend.process_frame, and differs from +0%
    fpn.nl_check.button.click()
    st.set_param("t_noise", 60.0)
    dst = os.path.join(outdir, "export_tnoise.mp4")
    video = win.src
    lo, n = 100, 2
    win.process_video(dst, start=lo, frames=n)
    ok = wait(app, lambda: win.export is None, 1800) and getattr(win, "export_ok", False)
    info = probe(dst)
    got = [f for _, f in read_frames(dst, info["W"], info["H"], fps=info["fps"])]
    sinfo = probe(video)
    P = st.backend()
    pj = json.load(open(os.path.splitext(dst)[0] + ".ui_params.json"))
    eprof = json.load(open(os.path.splitext(dst)[0] + ".ui_profile.json"))
    R = int(P["radius"])
    src = {i: tuple(p.astype(np.float32) for p in f)
           for i, f in read_frames(video, sinfo["W"], sinfo["H"], lo - R, lo + n + R, sinfo["fps"])}
    exact, differs = ok and len(got) == n and pj.get("t_noise") == 0.6, False
    for k in range(min(n, len(got))):
        c = lo + k
        ids = [i for i in sorted(src) if abs(i - c) <= R]
        out = backend.process_frame([src[i] for i in ids], ids.index(c), eprof, P)
        exact &= all(np.array_equal(np.clip(np.rint(o), 0, 255).astype(np.uint8), g) for o, g in zip(out, got[k]))
        o1 = backend.process_frame([src[i] for i in ids], ids.index(c), eprof, dict(P, t_noise=1.0))
        differs |= any(not np.array_equal(np.clip(np.rint(o), 0, 255).astype(np.uint8), g) for o, g in zip(o1, got[k]))
    st.undo.undo()
    check("NLC (c) export with Temporal Noise Level -40% (CRF 0) == backend.process_frame bit for bit, differs "
          "from +0%", exact and differs and st.params["t_noise"] == 100.0, f"{len(got)} frames {exact} {differs}")

    # (d) one state for the panel, Filter Settings Check > 1. Noise Level Check and Tools > Noise Level Check;
    # exclusive with Filter Tuning Assist; ends on the Prepare page / Reset Profile
    win.fsc_nlc.trigger()
    m_on = synced(True)
    win.fsc_btn.toggle()                                             # the button only shows the state
    btn_kept = synced(True)
    win.actions["nlc"].trigger()
    t_off = synced(False)
    fpn.nl_check.button.click()
    win.set_assist(True)
    excl = synced(False) and vw.assist and fpn.assist.button.isChecked()
    win.fsc_nlc.trigger()
    excl2 = synced(True) and not vw.assist and not fpn.assist.button.isChecked()
    win.tabs.setCurrentIndex(0)
    page_off = synced(False)
    win.actions["nlc"].trigger()
    page_back = synced(True) and win.page() == 1
    win.reset_profile()
    reset_off = synced(False) and not fpn.nl_check.isEnabled() and not win.fsc_nlc.isEnabled() \
        and not win.actions["nlc"].isEnabled()
    win.actions["nlc"].trigger()
    stays_off = synced(False)
    st.undo.undo()
    back = fpn.nl_check.isEnabled() and win.fsc_nlc.isEnabled() and win.actions["nlc"].isEnabled() and not win.nlc
    check("NLC (d) panel / toolbar menu / Tools menu in sync, toolbar 'Noise Level Check' pressed; exclusive with "
          "Filter Tuning Assist; ends on Prepare / Reset Profile; disabled without a profile",
          m_on and btn_kept and t_off and excl and excl2 and page_off and page_back and reset_off and stays_off
          and back, f"{m_on} {btn_kept} {t_off} {excl} {excl2} {page_off} {page_back} {reset_off} {stays_off} {back}")

    # R3 panel: yellow rows, other Temporal sections and the Spatial / General tabs disabled, restored on Finish
    th = fpn.headers["Temporal Filter"]
    ah = fpn.headers["Adapt to Changing Noise"]
    labs = (th.title, ah.title, fpn.controls["t_radius"].label, fpn.controls["t_noise"].label)
    texts0 = [(l.text(), l.styleSheet()) for l in labs]
    win.tabs.setCurrentIndex(1)
    fpn.tabs.setCurrentIndex(1)
    fpn.nl_check.button.click()
    yellow = all(ACTIVE_TITLE in l.text() or ACTIVE_TITLE in l.styleSheet() for l in labs)
    dis = (not fpn.tabs.isTabEnabled(1) and not fpn.tabs.isTabEnabled(2) and fpn.tabs.currentIndex() == 0
           and not fpn.controls["t_amount"].isEnabled() and not fpn.headers["Jitter of Details"].isEnabled()
           and not fpn.headers["Repeated Frames"].isEnabled() and fpn.controls["t_noise"].isEnabled()
           and fpn.controls["t_radius"].isEnabled() and fpn.controls["t_enable"].isEnabled()
           and fpn.t_noise_header.body.isVisible())
    fpn.grab().save(os.path.join(outdir, "nlc_panel.png"))
    fpn.nl_check.button.click()
    restored = ([(l.text(), l.styleSheet()) for l in labs] == texts0 and fpn.tabs.isTabEnabled(1)
                and fpn.tabs.isTabEnabled(2) and fpn.controls["t_amount"].isEnabled()
                and fpn.headers["Jitter of Details"].isEnabled() and not fpn.controls["flicker_local"].isEnabled()
                and not fpn.controls["t_quality"].isEnabled())
    check("NLC R3 panel: Temporal Filter / Radius / Noise Level / Adapt yellow, other sections and Spatial / "
          "General tabs disabled; restored on Finish (n/a rows stay disabled)", yellow and dis and restored,
          f"{yellow} {dis} {restored}")


def quality_checks(app, win, wait, idle, outdir):
    """Overall Profile Quality (2026-10-26, quality.py; decode + verification in
    sandbox/omo/coord/verify_quality/static/QUALITY.md)."""
    import copy
    import struct
    import numpy as np
    from nv_ui_qt.quality import genuine_g, profile_quality

    def blob(states, c2a4=1):
        b = bytearray(0xc300)
        b[0xbf34] = 1
        b[0xc2a4] = c2a4
        for (c, k), stv in states.items():
            o = 0xbf44 + 0x120 * c + 0x20 * k
            for wd in range(6):
                struct.pack_into("<I", b, o + 4 * wd,
                                 struct.unpack("<I", struct.pack("<f", stv.get("scale", 1.0)))[0])
            if stv.get("valid"):
                struct.pack_into("<I", b, o + 0x18, 1)
            b[o + 0x1c] = stv.get("marker", 0)
            b[o + 0x1d] = stv.get("clip", 0)
            b[o + 0x1e] = stv.get("gt3", 0)
        return bytes(b)

    def fresh(**kw):
        return {(c, k): dict(**kw) for c in range(3) for k in range(9)}

    g = float(genuine_g(blob(fresh())))
    check("Quality g: fresh profile = 1/81 (LZMA flag path only)", abs(g - 1 / 81) < 1e-9, f"{g}")
    stv = fresh()
    for key in stv:
        stv[key]["marker"] = 1
    g = float(genuine_g(blob(stv)))
    check("Quality g: after AFT = float32 27*(1/27)", abs(g - 0.999999762) < 1e-6, f"{g}")
    stv = fresh()
    for key in stv:
        stv[key]["marker"] = 1
    stv[(1, 4)]["gt3"] = 1
    stv[(2, 7)]["gt3"] = 1
    g = float(genuine_g(blob(stv)))
    check("Quality g: two >3.0 flags -> 25*(+1/27) + 2*(-1/27) = 23/27",
          abs(g - 23 / 27) < 1e-6, f"{g}")
    stv = fresh()
    stv[(0, 3)]["clip"] = 1
    check("Quality g: clip flag clamps to 0 (1/81 - 1/27 < 0)", float(genuine_g(blob(stv))) == 0.0)
    stv = fresh(scale=0.0)
    check("Quality g: all-empty clamps to 0", float(genuine_g(blob(stv))) == 0.0)
    stv = fresh()
    stv[(0, 5)]["scale"] = 0.0                      # (0,5) itself -1/81
    stv[(2, 8)]["clip"] = 1                         # -1/27, kills the 0xc2a4 +1/81 path
    check("Quality g: zero scale word + clip flag -> -4/81 clamps to 0",
          float(genuine_g(blob(stv))) == 0.0)

    # the label: profile exists (auto-AFT ran), frames in memory
    st = win.state
    check("Quality: panel shows a percent for the AFT profile", wait(
        app, lambda: idle() and win.profile_page.panel.quality.text() not in ("", "n/a")),
        win.profile_page.panel.quality.text())
    pct_aft = profile_quality(st.profile, win.frames, win.full_range, win.matrix)
    check("Quality: label matches recomputation",
          win.profile_page.panel.quality.text() == f"{pct_aft}%", f"{pct_aft}")
    # AFT stores per-bin flags; markers only where the AFT run left them (measured bins
    # keep 0). All-marked happens when nothing was measured (small/flat area); partial marking
    # otherwise. The genuine fit (23/27 + exact +1/81 series) is in QUALITY.md 8.4-8.5.
    flags = st.profile.base.get("aft_bin_flags")
    n_marked = sum(1 for ch in flags["marker"] for v in ch) if flags else -1
    check("Quality: AFT bin flags stored (3x9 bools each)",
          flags is not None and len(flags["marker"]) == 3
          and all(len(ch) == 9 for ch in flags["marker"])
          and all(isinstance(v, bool) for ch in flags["marker"] for v in ch)
          and 0 <= n_marked <= 27,
          f"marked {n_marked}/27")
    from nv_ui_qt.quality import profile_completeness, profile_numbers
    comp_aft = profile_completeness(st.profile, win.frames, win.full_range, win.matrix)
    check("Completeness: panel shows g x 100 for the AFT profile", wait(
        app, lambda: idle() and win.profile_page.panel.completeness.text() not in ("", "n/a")),
        win.profile_page.panel.completeness.text())
    check("Completeness: label matches recomputation",
          win.profile_page.panel.completeness.text() == f"{comp_aft}%", f"{comp_aft}")
    qp, cp = profile_numbers(st.profile, win.frames, win.full_range, win.matrix)
    check("Completeness: profile_numbers agrees with both labels", qp == pct_aft and cp == comp_aft)
    fresh_ps = copy.deepcopy(st.profile)
    fresh_ps.base = copy.deepcopy(st.profile.base)
    fresh_ps.base.pop("aft_applied", None)
    fresh_ps.base.pop("ui_pre_aft", None)
    pct_fresh = profile_quality(fresh_ps, win.frames, win.full_range, win.matrix)
    comp_fresh = profile_completeness(fresh_ps, win.frames, win.full_range, win.matrix)
    check("Quality: AFT raises the value (fresh s11 ~0.506 -> marked bins)", pct_fresh is not None and
          pct_fresh < pct_aft, f"fresh {pct_fresh} < aft {pct_aft}")
    check("Completeness: fresh profile is 0-1% (unmarked bins, LZMA flag path at most)",
          comp_fresh is not None and comp_fresh in (0, 1), f"{comp_fresh}")
    # Reset Fine-Tuning: aft_reset clears the LZMA flag (0xc2a4) -> g = 0 -> Completeness 0%
    reset_ps = copy.deepcopy(st.profile)
    reset_ps.base = copy.deepcopy(st.profile.base)
    reset_ps.factors = [[1.0] * 9 for _ in range(3)]
    reset_ps.base["aft_reset"] = True
    reset_ps.base.pop("aft_bin_flags", None)
    qp_r, cp_r = profile_numbers(reset_ps, win.frames, win.full_range, win.matrix)
    check("Completeness: after Reset the LZMA flag is cleared -> 0%",
          cp_r == 0, f"{cp_r}")
    check("Quality: after Reset only the curve term is gone (s11 = 0.5)",
          qp_r is not None and 0 < qp_r < pct_aft, f"{qp_r}")
    none_ps = copy.deepcopy(st.profile)
    none_ps.base = None
    check("Quality: n/a without a profile", profile_quality(none_ps, win.frames, win.full_range,
                                                            win.matrix) is None
          and profile_quality(st.profile, None, win.full_range, win.matrix) is None
          and profile_completeness(none_ps, win.frames, win.full_range, win.matrix) is None)
    # the Profile Viewer lines
    dlg = win.profile_viewer()
    txt = dlg.findChildren(__import__("PySide6.QtWidgets", fromlist=["QTextEdit"]).QTextEdit)[0].toPlainText()
    dlg.close()
    check("Quality: Profile Viewer shows the value",
          f"Overall Profile Quality: {pct_aft}%" in txt, txt.splitlines()[-2:])
    check("Completeness: Profile Viewer shows the value",
          f"Completeness: {comp_aft}%" in txt, txt.splitlines()[-2:])


def main(video, outdir):
    os.makedirs(outdir, exist_ok=True)
    os.environ["NV_UI_QT_SETTINGS"] = os.path.join(outdir, "settings.ini")
    if os.path.exists(os.environ["NV_UI_QT_SETTINGS"]):
        os.remove(os.environ["NV_UI_QT_SETTINGS"])
    app = QApplication([sys.argv[0]])
    import nvfilter as nv
    from denoise_video_v5 import probe, read_frames
    from nv_ui_qt import backend
    from nv_ui_qt.main_window import MainWindow, settings
    from nv_ui_qt.state import PARAM_SPECS, default_params

    s = settings()
    s.setValue("prefs/crf", 0)
    s.setValue("prefs/jobs", 3)
    s.sync()
    win = MainWindow(video)
    win.show()
    st = win.state
    idle = lambda: not win.runner.running and not win.runner.queued
    check("load + auto profile", wait(app, lambda: st.profile.base is not None and idle()),
          f"{len(win.frames or [])} frames, area {st.profile.base and st.profile.base['area']}")
    first_modes = [win.viewer.mode()]
    win.tabs.setCurrentIndex(1)                 # Adjust and Preview: the filtered preview is computed
    first_modes.append(win.viewer.mode())
    win.first_modes = first_modes               # checked in genuine_ui_checks (genuine defaults per page)
    check("preview filtered", wait(app, lambda: idle() and win.filt_cache and win.viewer.filt is not None))

    # defaults = V5_DEFAULTS
    P = st.backend()
    D = nv.V5_DEFAULTS
    same = all(P[k] == D[k] for k in D)
    check("UI defaults map to nv.V5_DEFAULTS", same)

    # bit-exact vs process_frame_v5 (default, uniform channels)
    t = win.timeline.frame()
    prof = st.effective_profile()
    R = int(P["radius"])
    idx, win_planes = win.engine.window(t, R, None)
    ci = idx.index(t)
    ref = nv.process_frame_v5(win_planes, ci, prof, nv.v5_params())
    got = backend.process_frame(win_planes, ci, prof, P)
    check("backend.process_frame == nv.process_frame_v5 (defaults)",
          all(np.array_equal(a, b) for a, b in zip(ref, got)))

    # per-channel band setting: only Cb changes
    ui = default_params()
    ui["bamt_Cb_medium"] = 50.0
    from nv_ui_qt.state import backend_params
    got2 = backend.process_frame(win_planes, ci, prof, backend_params(ui))
    check("per-channel band: Y and Cr unchanged, Cb changed",
          np.array_equal(got2[0], ref[0]) and np.array_equal(got2[2], ref[2]) and not np.array_equal(got2[1], ref[1]))

    sub420 = win.frames[0][1].shape != win.frames[0][0].shape
    fpc = win.filter_page.panel.controls
    check("chroma 'High' band sliders follow the profile's band count",
          (not fpc["bamt_Cb_high"].isEnabled() and not fpc["bnoise_Cr_high"].isEnabled() and fpc["bamt_Y_high"].isEnabled())
          if sub420 and prof["levels"]["Cb"] < 5 else fpc["bamt_Cb_high"].isEnabled())

    # slider -> state -> preview update
    gen0 = win.gen
    w = win.filter_page.panel.controls["amt_Y"]
    w.slider.setValue(40)
    check("slider edits state", abs(st.params["amt_Y"] - 40.0) < 1e-9)
    check("preview recomputed after slider", wait(app, lambda: idle() and win.gen > gen0 and
                                                  any(k[1] == win.gen for k in win.filt_cache)))
    # every implemented control reaches the backend dict
    base_P = backend_params(default_params())
    missing = []
    deps = {"mix": {"mix_enable": True}, "mix_enable": {"mix": 25.0}}   # mix acts only with its Enable
    for k, sp in PARAM_SPECS.items():
        if not sp.implemented:
            continue
        ui = default_params()
        ui.update(deps.get(k, {}))
        if sp.kind == "bool":
            ui[k] = not ui[k]
        elif sp.kind == "choice":
            ui[k] = [c for c in sp.choices if c != ui[k]][0]
        elif sp.kind == "int":
            ui[k] = int(sp.lo) if ui[k] != sp.lo else int(sp.hi)
        else:
            ui[k] = sp.lo if ui[k] != sp.lo else sp.hi
        if json.dumps(backend_params(ui), sort_keys=True, default=str) == json.dumps(base_P, sort_keys=True, default=str):
            missing.append(k)
    check("every implemented control changes the backend parameters", not missing, str(missing))
    disabled = [k for k, sp in PARAM_SPECS.items() if not sp.implemented]
    check("unimplemented controls are disabled",
          all(not win.filter_page.panel.controls[k].isEnabled() for k in disabled if k in win.filter_page.panel.controls),
          f"{len(disabled)} controls")

    # undo / redo
    st.undo.undo()
    check("undo restores value and widget", st.params["amt_Y"] == 70.0 and abs(w.value() - 70.0) < 1e-9)
    st.undo.redo()
    check("redo", st.params["amt_Y"] == 40.0 and abs(w.value() - 40.0) < 1e-9)
    st.undo.undo()

    # presets
    st.set_param("noise_Cr", 150.0)
    pp = os.path.join(outdir, "p.nvpreset.json")
    win.save_preset(pp)
    win.reset_filter()
    check("reset filter", st.params["noise_Cr"] == 100.0)
    win.load_preset(pp)
    check("preset save/load", st.params["noise_Cr"] == 150.0 and
          abs(win.filter_page.panel.controls["noise_Cr"].value() - 150.0) < 1e-9)
    win.assign_default_preset()
    win.reset_filter()
    check("reset uses assigned default preset", st.params["noise_Cr"] == 150.0)
    settings().remove("default_preset")
    win.reset_filter()

    # profile: AFT (wait for the auto-AFT triggered by the earlier Auto Profile to finish first)
    wait(app, lambda: idle() and not win.runner.busy("aft"), 120)
    # auto-AFT already changed the curve; the explicit AFT button re-runs it (same result)
    win.auto_fine_tune()
    check("Auto Fine-Tune runs and curve is AFT-adjusted",
          wait(app, lambda: idle() and st.profile.base.get("aft_applied")))
    idx_before = st.undo.index()
    st.undo.undo()
    check("undo AFT", st.undo.index() < idx_before and st.profile.base is not None
          and "curve" in st.profile.base)
    st.undo.redo()
    once = json.dumps(st.profile.base["curve"])
    win.auto_fine_tune()
    check("Auto Fine-Tune twice == once (no squared factors)",
          wait(app, lambda: idle()) and json.dumps(st.profile.base["curve"]) == once)
    # profile replaced while AFT runs -> stale AFT result discarded
    saved = st.profile.clone()
    win.auto_fine_tune()
    wait(app, lambda: win.runner.busy("aft"), 10)
    win.reset_profile()
    wait(app, lambda: idle())
    check("stale AFT result discarded", st.profile.base is None)
    st.undo.undo()
    check("AFT buttons re-enabled", win.profile_page.panel.b_aft.isEnabled())
    # cancel: a running AFT stops promptly
    win.auto_fine_tune()
    wait(app, lambda: win.runner.busy("aft"), 10)
    t0 = time.time()
    win.runner.wait_all(30000)
    app.processEvents()
    check("AFT cancellable (child process terminated)", time.time() - t0 < 5 and not win.runner.busy("aft"),
          f"{time.time() - t0:.1f}s")
    wait(app, lambda: idle())
    # malformed files are rejected without changing the profile
    bad = os.path.join(outdir, "bad.json")
    good = json.dumps(st.profile.to_json())
    for content in ("[1, 2]", "3", json.dumps({"format": "nv_repro_profile", "base": {"x": 1}})):
        open(bad, "w").write(content)
        win.load_profile(bad)
        win.load_preset(bad)
    check("malformed profile / preset files rejected", json.dumps(st.profile.to_json()) == good)

    # manual fine-tune + auto complete
    ed = win.profile_page.panel.curve
    st.new_edit()
    ed.edited.emit(0, [1.0, 1.0, 1.5, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0], 2)
    st.new_edit()
    ed.edited.emit(0, [1.0, 1.0, 1.5, 1.0, 1.0, 1.0, 0.5, 1.0, 1.0], 6)
    win.auto_complete()
    f = st.profile.factors[0]
    check("manual fine-tune + auto complete", abs(f[2] - 1.5) < 1e-9 and abs(f[6] - 0.5) < 1e-9 and
          abs(f[4] - 1.0) < 1e-9 and abs(f[0] - 1.5) < 1e-9 and abs(f[8] - 0.5) < 1e-9, str(f))
    eff = st.effective_profile()
    check("fine-tune changes the effective curve", eff["curve"]["Y"] != st.profile.base["curve"]["Y"])
    pf = os.path.join(outdir, "prof.nvprof.json")
    win.save_profile(pf)
    win.reset_profile()
    check("reset profile", st.profile.base is None)
    win.load_profile(pf)
    check("profile save/load", st.profile.factors[0] == f and st.effective_profile()["curve"] == eff["curve"])

    # build profile from a dragged area / most noisy frame
    win.on_area(8, 8, 64)
    box = win.build_profile(False)              # 32..127 px: the genuine asks first (Use the small area)
    asked = box is not None and [b.text() for b in box.buttons()].count("Use the small area") == 1
    [b for b in box.buttons() if b.text() == "Use the small area"][0].click()
    check("Build Profile (Current Frame), small-area confirmation", asked and wait(
        app, lambda: idle() and st.profile.base["area"][:3] == [8, 8, 64]))
    win.auto_profile(True)
    nf = backend.noisiest_frame(win.frames)
    check("Auto Profile (Most Noisy Frame)", wait(app, lambda: idle() and st.profile.base["ui_frame"] == nf
                                                  and st.profile.base.get("flat_status") is not None), f"frame {nf}")

    # views: viewer modes, Assist, save frame, preview area
    from nv_ui_qt.widgets.viewer import MODES, SPLITS
    win.tabs.setCurrentIndex(1)
    wait(app, lambda: idle() and win.viewer.showing_filtered())
    for m, _ in MODES:
        win.viewer.set_mode(m)
        win.viewer.grab()
    win.set_assist(True)
    win.viewer.grab()
    win.set_assist(False)
    win.viewer.set_mode("YCrCb +")
    png = os.path.join(outdir, "frame.png")
    win.save_frame(png)
    check("preview modes + save frame", os.path.getsize(png) > 0)
    win.area_btn.setChecked(True)
    check("Preview Area (crop) filtering", wait(app, lambda: idle() and any(k[2] is not None for k in win.filt_cache)))
    win.area_btn.setChecked(False)
    check("preview area off -> whole frame", win.crop() is None and win.preview_sel is None)
    win.profile_viewer().close()
    win.guide("filter").close()

    # ---------------- layout checks ----------------
    fpn, ppn = win.filter_page.panel, win.profile_page.panel
    from nv_ui_qt.widgets.rows import Header, Row

    def page_headers(i):
        lay = fpn.tabs.widget(i).widget().layout()
        return [lay.itemAt(j).widget().title_text for j in range(lay.count())
                if isinstance(lay.itemAt(j).widget(), Header)]

    def body_labels(h):
        return [h.lay.itemAt(j).widget().label.text() for j in range(h.lay.count())
                if isinstance(h.lay.itemAt(j).widget(), Row)]

    def body_headers(h):
        return {h.lay.itemAt(j).widget().title_text: h.lay.itemAt(j).widget() for j in range(h.lay.count())
                if isinstance(h.lay.itemAt(j).widget(), Header)}

    tabs = [fpn.tabs.tabText(i) for i in range(fpn.tabs.count())]
    nl = fpn.headers["Noise Level"]
    nra = [h for k, h in fpn.headers.items() if k.startswith("Noise Reduction Amount") and
           "Chrominance" in body_headers(h)][0]
    order_ok = (tabs == ["Temporal", "Spatial", "General"]
                and page_headers(0) == ["Temporal Filter", "Temporal Noise", "Repeated Frames", "Jitter of Details",
                                        "Flicker", "Dust and Scratches", "Adapt to Changing Noise"]
                and "Noise Reduction Amount" in body_headers(fpn.headers["Temporal Noise"])
                and page_headers(1) == ["Spatial Filter", "Noise Reduction Amount", "Noise Level", "Artifact Removal",
                                        "Edge Smoothing", "Sharpening", "Per-component bands (repro)", "Advanced (repro)"]
                and page_headers(2) == ["Mix with Original", "Performance"]
                and body_labels(nl) == ["Luminance", "Cr", "Cb"]
                and body_labels(body_headers(nl)["Frequencies"]) == ["High", "Mid", "Low", "Very Low", "Ultra Low"]
                and body_labels(nra) == ["Luminance"]
                and body_labels(body_headers(nra)["Chrominance"]) == ["Cr", "Cb"]
                and body_labels(body_headers(nra)["Frequencies"]) == ["High", "Mid", "Low", "Very Low", "Ultra Low"])
    pbody = ppn.curve.parentWidget().parentWidget().layout()
    idx = lambda w: pbody.indexOf(w)
    tuning = [w for w in ppn.findChildren(Header) if w.title_text == "Profile Check and Tuning"][0]
    stats = ppn.stats
    prep_ok = idx(stats) < idx(ppn.curve.parentWidget()) < idx(tuning) and \
        [w.label.text() for w in (ppn.tune["Y"], ppn.tune["Cr"], ppn.tune["Cb"])] == ["Luminance", "Cr", "Cb"]
    check("(a) panel order: Temporal / Spatial / General, Luminance -> Cr -> Cb, High -> Ultra Low",
          order_ok and prep_ok, f"{page_headers(1)}")

    # (b) common Frequencies row: three components, one undo step
    row = [w for w in fpn.common if w.keys[0] == "bamt_Y_high"][0]
    keys = row.keys
    n0 = st.undo.index()
    st.new_edit()
    row.slider.setValue(60)
    row.slider.setValue(55)
    one = st.undo.index() == n0 + 1 and all(st.params[k] == 55.0 for k in keys)
    st.undo.undo()
    back = all(st.params[k] == 100.0 for k in keys) and row.value() == 100.0
    st.set_param("bamt_Cb_medium", 50.0)
    mrow = [w for w in fpn.common if w.keys[0] == "bamt_Y_medium"][0]
    mixed = mrow.val.text() == "mixed"
    st.undo.undo()
    check("(b) Frequencies row writes Y / Cr / Cb in one undo step", one and back and mixed and
          sorted(keys) == ["bamt_Cb_high", "bamt_Cr_high", "bamt_Y_high"] and mrow.val.text() == "100%")

    # (c) Mix with Original: Enable, display 100 - mix, old presets
    me = fpn.controls["mix_enable"]
    off = st.backend()["mix"] == 0.0 and not st.params["mix_enable"]
    n0 = st.undo.index()
    me.setChecked(True)
    on = (st.params["mix_enable"] and st.params["mix"] == 25.0 and st.backend()["mix"] == 0.25
          and fpn.controls["mix"].val.text() == "75%" and st.undo.index() == n0 + 1)
    me.setChecked(False)
    dis = st.backend()["mix"] == 0.0 and st.params["mix"] == 25.0
    st.undo.undo()
    st.undo.undo()
    old = os.path.join(outdir, "old_mix.json")
    json.dump({"format": "nv_repro_filter_preset", "version": 1, "params": {"mix": 30.0}}, open(old, "w"))
    win.load_preset(old)
    old_on = st.params["mix_enable"] and st.backend()["mix"] == 0.3
    json.dump({"format": "nv_repro_filter_preset", "version": 1, "params": {"mix": 0.0}}, open(old, "w"))
    win.load_preset(old)
    old_off = not st.params["mix_enable"]
    win.reset_filter()
    check("(c) mix_enable: off by default, Enable -> 75%, old presets", off and on and dis and old_on and old_off
          and st.params == default_params(), f"{off} {on} {dis} {old_on} {old_off} "
          f"{[k for k in st.params if st.params[k] != default_params()[k]]}")

    # (d) profile Noise Level (tune): multiplies the curve, undo, JSON
    base_c = list(st.effective_profile()["curve"]["Y"])
    st.new_edit()
    ppn.tune["all"].slider.setValue(50)
    eff2 = st.effective_profile()
    scaled = st.profile.tune_value("all") == 1.5 and np.allclose(eff2["curve"]["Y"], np.asarray(base_c) * 1.5, rtol=1e-5)
    st.new_edit()
    ppn.tune["Cr"].slider.setValue(-50)
    cr_ok = np.allclose(st.effective_profile()["curve"]["Cr"],
                        np.asarray(st.profile.base["curve"]["Cr"]) * 0.75 * np.interp(
                            st.profile.base["curve_centers"], [32.0 * k for k in range(9)], st.profile.factors[2]),
                        rtol=1e-5)
    tp = os.path.join(outdir, "tune.nvprof.json")
    win.save_profile(tp)
    saved = json.load(open(tp))["tune"] == {"all": 1.5, "Cr": 0.5}
    st.undo.undo()
    st.undo.undo()
    undone = st.profile.tune_value("all") == 1.0 and st.effective_profile()["curve"]["Y"] == base_c
    from nv_ui_qt.state import ProfileState
    d = json.load(open(tp))
    d.pop("tune")
    legacy = ProfileState.from_json(d).tune_value("all") == 1.0
    win.load_profile(tp)
    loaded = st.profile.tune_value("Cr") == 0.5 and ppn.tune["Cr"].val.text() == "-50%"
    st.undo.undo()
    check("(d) profile Noise Level scales the curve, undo, saved in JSON", scaled and cr_ok and saved and undone
          and legacy and loaded)

    # (e) hold the left button = original, "Filtered" otherwise
    win.tabs.setCurrentIndex(1)
    wait(app, lambda: idle() and win.viewer.showing_filtered())
    vw = win.viewer
    c = vw.panes()[0][1].center().toPoint()
    shown = vw.showing_filtered()
    QTest.mousePress(vw, Qt.LeftButton, Qt.NoModifier, c)
    held = not vw.showing_filtered() and vw.hold
    QTest.mouseRelease(vw, Qt.LeftButton, Qt.NoModifier, c)
    check("(e) press-and-hold shows the original ('Filtered' label off)", shown and held and vw.showing_filtered())

    # (f) YCrCb+ panes and splits
    vw.set_mode("YCrCb +")
    geo = {}
    for sp in SPLITS:
        vw.set_split(sp)
        geo[sp] = [r for _, r in vw.panes()]
    img = vw.grab().toImage()
    b, r2 = geo["1+3 Bottom"], geo["1+3 Right"]
    g2 = geo["2 x 2"]
    names = [n for n, _ in vw.panes()]
    vw.set_mode("Normal")
    normal = len(vw.panes()) == 1
    vw.set_mode("YCrCb +")
    vw.set_split("1+3 Bottom")
    check("(f) YCrCb+ draws 4 panes; split changes the arrangement",
          names == ["rgb", "Y+", "Cr+", "Cb+"] and normal and not img.isNull()
          and all(q.top() > b[0].bottom() for q in b[1:]) and all(q.left() > r2[0].right() for q in r2[1:])
          and abs(g2[0].width() - g2[1].width()) < 1 and abs(g2[0].height() - g2[2].height()) < 1
          and abs(b[0].height() / (b[0].height() + b[1].height()) - 2 / 3) < 0.02)

    # (g) thumbnails follow the Radius; click selects the frame
    th = win.timeline.thumbs
    cur = win.timeline.frame()
    blue, orange = th.frame_color(cur), th.frame_color(cur + 2)
    st.set_param("t_radius", 1)
    r1 = th.frame_color(cur + 1) is not None and th.frame_color(cur + 2) is None and th.frame_color(cur - 1) is not None
    st.set_param("t_enable", False)
    r0 = th.frame_color(cur + 1) is None
    st.undo.undo()
    st.undo.undo()
    tgt = [rr for i, rr in th.slots() if i == cur + 3][0]
    QTest.mouseClick(th, Qt.LeftButton, Qt.NoModifier, tgt.center().toPoint())
    moved = win.timeline.frame() == cur + 3
    win.timeline.set_frame(cur, emit=True)
    check("(g) thumbnails: blue = current, orange = Radius window, click = go to frame",
          blue is not None and orange is not None and blue != orange and r1 and r0 and moved)

    # (h) Beginner / Advanced: rows hidden, values kept
    st.set_param("noise_Cr", 120.0)
    win.set_advanced(False)
    beg = (fpn.controls["noise_Cr"].isHidden() and not fpn.controls["amt_Y"].isHidden()
           and fpn.headers["Mix with Original"].isHidden() and not fpn.headers["Performance"].isHidden()
           and not fpn.controls["adv_noise_level"].isHidden() and ppn.curve.parentWidget().isHidden()
           and fpn.sharp_rows[0].label.text() == "Fine" and st.params["noise_Cr"] == 120.0)
    win.set_advanced(True)
    adv = not fpn.controls["noise_Cr"].isHidden() and fpn.controls["noise_Cr"].value() == 120.0 and \
        not ppn.curve.parentWidget().isHidden() and fpn.sharp_rows[0].label.text() == "High"
    st.undo.undo()
    check("(h) Beginner / Advanced Mode hides rows and keeps values", beg and adv)

    # drag = one undo step, page / wheel steps = separate steps (D6); both slider kinds
    from PySide6.QtWidgets import QAbstractSlider
    drags = []
    for w, get in ((fpn.controls["amt_Y"], lambda: st.params["amt_Y"]),
                   (ppn.tune["all"], lambda: st.profile.tune_value("all"))):
        sl = w.slider
        i0 = st.undo.index()
        v0 = sl.value()
        sl.setSliderDown(True)                  # sliderPressed -> new undo step
        for dv in (3, 6, 9, 12, 15):            # setSliderPosition while down -> actionTriggered(SliderMove)
            sl.setSliderPosition(v0 + dv)
        sl.setSliderDown(False)
        one = st.undo.index() == i0 + 1 and get() != (70.0 if w is fpn.controls["amt_Y"] else 1.0)
        sl.triggerAction(QAbstractSlider.SliderAction.SliderPageStepSub)
        sl.triggerAction(QAbstractSlider.SliderAction.SliderPageStepSub)
        steps = st.undo.index() == i0 + 3
        for _ in range(3):
            st.undo.undo()
        drags.append(one and steps and st.undo.index() == i0)
    check("slider drag = one undo step, page steps = separate steps", all(drags), str(drags))

    # wheel steps never merge into a drag or into each other (D6)
    from PySide6.QtCore import QPoint, QPointF
    from PySide6.QtGui import QWheelEvent

    def wheel(sl):
        c = QPointF(sl.width() / 2, sl.height() / 2)
        QApplication.sendEvent(sl, QWheelEvent(c, sl.mapToGlobal(c), QPoint(0, 0), QPoint(0, 120), Qt.NoButton,
                                               Qt.NoModifier, Qt.NoScrollPhase, False))
        app.processEvents()

    sl = fpn.controls["amt_Y"].slider
    i0 = st.undo.index()
    sl.setSliderDown(True)
    for dv in (-5, -10, -20):
        sl.setSliderPosition(70 + dv)
    sl.setSliderDown(False)
    drag1 = st.undo.index() == i0 + 1 and st.params["amt_Y"] == 50.0
    wheel(sl)
    drag_wheel = st.undo.index() == i0 + 2 and st.params["amt_Y"] != 50.0
    time.sleep(0.3)
    wheel(sl)
    two_wheels = st.undo.index() == i0 + 3
    st.undo.undo()
    st.undo.undo()
    back_to_drag = st.params["amt_Y"] == 50.0
    st.undo.undo()
    check("wheel: drag = 1, drag + wheel = 2, two wheel gestures = 2 undo steps",
          drag1 and drag_wheel and two_wheels and back_to_drag and st.params["amt_Y"] == 70.0,
          f"{drag1} {drag_wheel} {two_wheels} {back_to_drag}")

    # no side-panel row is wider than its viewport at the minimum window width (all sections open)
    from PySide6.QtWidgets import QScrollArea
    size0 = win.size()
    for hh in win.findChildren(Header):
        hh.set_expanded(True)
    win.resize(win.minimumSizeHint().width(), size0.height())
    over = []
    for adv in (True, False):
        win.set_advanced(adv)
        for page in (0, 1):
            win.tabs.setCurrentIndex(page)
            panel = win.stack.currentWidget().panel
            scs = [panel.tabs.widget(i) for i in range(panel.tabs.count())] if page else \
                panel.findChildren(QScrollArea)
            for i, sc in enumerate(scs):
                if page:
                    panel.tabs.setCurrentIndex(i)
                for _ in range(3):
                    app.processEvents()
                if sc.widget().minimumSizeHint().width() > sc.viewport().width():
                    over.append((adv, page, i, sc.widget().minimumSizeHint().width(), sc.viewport().width()))
    win.set_advanced(True)
    win.resize(size0)
    app.processEvents()
    check("side panels fit at the minimum window width (all sections expanded, both modes)", not over,
          f"min width {win.minimumSizeHint().width()} {over}")

    # non-finite profile values are rejected
    good = json.dumps(st.profile.to_json())
    bad = os.path.join(outdir, "bad_tune.json")
    rej = []
    for mut in ("tune", "factor", "curve"):
        e = json.loads(good)
        if mut == "tune":
            e["tune"] = {"all": float("nan")}
        elif mut == "factor":
            e["factors"][0][3] = float("inf")
        else:
            e["base"]["curve"]["Y"][0] = float("nan")
        json.dump(e, open(bad, "w"))
        win.load_profile(bad)
        rej.append(json.dumps(st.profile.to_json()) == good)
    check("profile JSON with NaN / inf tune, factors or curve rejected", all(rej), str(rej))

    # every non-implemented control shows visible "n/a"
    from nv_ui_qt.widgets.rows import ChoiceRow, EnableCheck
    def na_text(w):
        if isinstance(w, Row):
            return w.val.text()
        if isinstance(w, EnableCheck):
            return w.na_label.text() if w.na_label is not None else w.text()
        if isinstance(w, ChoiceRow):
            return w.label.text()
        return ""
    widgets = [w for k, w in fpn.controls.items()] + [w for ws in fpn.extra.values() for w in ws]
    no_na = [w.key for w in widgets if not getattr(w, "spec", PARAM_SPECS.get(getattr(w, "key", ""))).implemented
             and "n/a" not in na_text(w)]
    # (Auto Match is no longer a toolbar button: the genuine Prepare toolbar has none; it stays in the menu)
    check("non-implemented controls show 'n/a'", not no_na and "n/a" in win.actions["p_match"].text()
          and not win.actions["p_match"].isEnabled() and "n/a" in ppn.b_manual.text(), str(no_na))

    # genuine slider tracks / texts
    nr, ov = fpn.controls["noise_Y"].slider, fpn.controls["adv_noise_level"]
    pos0 = (0 - nr.minimum()) / (nr.maximum() - nr.minimum())
    mix_txt = fpn.controls["mix"].val.text()
    win.tabs.setCurrentIndex(0)
    prep_r = win.timeline.thumbs.radius
    win.tabs.setCurrentIndex(1)
    check("Noise Level track -100..+150, Mix off shows 75%, Radius '2', no orange frames on Prepare",
          (nr.minimum(), nr.maximum()) == (-100, 150) and abs(pos0 - 0.4) < 1e-9
          and (ov.slider.minimum(), ov.slider.maximum()) == (-100, 150) and ov.spin.maximum() == 300
          and PARAM_SPECS["noise_Y"].hi == 250 and mix_txt == "75%" and not st.params["mix_enable"]
          and fpn.controls["t_radius"].val.text() == "2" and prep_r == 0 and win.timeline.thumbs.radius == 2)

    # (i) Auto Profile button becomes Build Profile with a selection
    win.tabs.setCurrentIndex(0)
    win.on_area(16, 16, 64)
    named = win.auto_btn.text() == "Build Profile" and win.viewer.sel_label.startswith("Noise Level:")
    win._viewer_click()
    check("(i) Auto Profile <-> Build Profile with the selection", named and win.auto_btn.text() == "Auto Profile"
          and win.sel_area is None)
    win.tabs.setCurrentIndex(1)

    genuine_ui_checks(app, win, wait, idle, outdir)
    profile_check_checks(app, win, wait, idle, outdir)
    nlc_checks(app, win, wait, idle, outdir)
    quality_checks(app, win, wait, idle, outdir)

    # batch profiler
    win.batch_profiler([video], outdir, "auto")
    check("batch profiler", wait(app, lambda: idle() and getattr(win, "batch_result", None)))

    # rectangular profile (216 x 512) built from the UI selection, then exported with it
    win.tabs.setCurrentIndex(0)
    H0, W0 = win.frames[0][0].shape
    rh, rw = min(216, H0 - 40), min(512, W0 - 60)
    win.on_area(40, 60, rh, rw)
    win.build_profile(False)
    check("rectangular Build Profile: area [y0, x0, h, w] kept", wait(
        app, lambda: idle() and st.profile.base["area"] == [40, 60, rh, rw] and st.profile.base.get("aft_applied")),
          f"{st.profile.base['area']}")
    win.tabs.setCurrentIndex(1)

    # export: lossless, compared with process_frame_v5 using the UI profile
    st.set_param("bamt_Cr_medium", 60.0)        # non-uniform channels exercise backend._work
    dst = os.path.join(outdir, "export.mp4")
    lo, n = 300, 4
    win.process_video(dst, start=lo, frames=n)
    ok = wait(app, lambda: win.export is None, 1800)
    check("export finished", ok and getattr(win, "export_ok", False))
    check("export used the rectangular profile", json.load(open(os.path.splitext(dst)[0] + ".ui_profile.json"))
          ["area"] == [40, 60, rh, rw])
    info = probe(dst)
    got = [f for _, f in read_frames(dst, info["W"], info["H"], fps=info["fps"])]
    sinfo = probe(video)
    P = st.backend()
    prof = json.load(open(os.path.splitext(dst)[0] + ".ui_profile.json"))
    R = int(P["radius"])
    src = {i: tuple(p.astype(np.float32) for p in f)
           for i, f in read_frames(video, sinfo["W"], sinfo["H"], lo - R, lo + n + R, sinfo["fps"])}
    exact = len(got) == n
    for k in range(min(n, len(got))):
        c = lo + k
        ids = [i for i in sorted(src) if abs(i - c) <= R]
        out = backend.process_frame([src[i] for i in ids], ids.index(c), prof, P)
        exp = [np.clip(np.rint(o), 0, 255).astype(np.uint8) for o in out]
        exact &= all(np.array_equal(a, b) for a, b in zip(exp, got[k]))
    check("export (CRF 0) == backend.process_frame bit for bit", exact, f"{len(got)} frames")

    # Preferences > ffmpeg (G2): a path without ffmpeg / ffprobe -> warning, the dialog stays open, nothing saved;
    # the ffmpeg binary of the tools found -> saved, used for this process (ffmpeg_path), a video opens
    from PySide6.QtWidgets import QLineEdit, QMessageBox, QPushButton
    from nv_ui_qt import ffmpeg_path
    good = ffmpeg_path.ffmpeg_dir()
    dlg = win.preferences()
    ffs = [e for e in dlg.findChildren(QLineEdit) if e.toolTip() == ffmpeg_path.TOOLTIP]
    browse = [b for b in dlg.findChildren(QPushButton) if b.text() == "Browse…"]
    bad = os.path.join(outdir, "no_ffmpeg_here", "ffmpeg")
    kept = saved = False
    if ffs and good:
        ffs[0].setText(bad)
        dlg.accept()
        app.processEvents()
        boxes = [b for b in dlg.findChildren(QMessageBox) if b.isVisible()]   # (macOS: no window title shown)
        kept = (dlg.isVisible() and len(boxes) == 1 and boxes[0].text() == ffmpeg_path.missing_text(bad)
                and settings().value("prefs/ffmpeg") is None)
        for b in boxes:
            b.accept()
        ffs[0].setText(os.path.join(good, "ffmpeg"))
        dlg.accept()
        closed = not dlg.isVisible()
        app.processEvents()
        saved = (closed and str(settings().value("prefs/ffmpeg")) == os.path.join(good, "ffmpeg")
                 and ffmpeg_path.ffmpeg_dir() == good and os.environ["PATH"].split(os.pathsep)[0] == good)
    win.open_video(video)
    opened = wait(app, lambda: win.src == video and st.profile.base is not None and idle())
    settings().remove("prefs/ffmpeg")
    check("Preferences ffmpeg row: a path without ffmpeg / ffprobe warns and keeps the dialog (nothing saved); the "
          "ffmpeg binary is saved and used, a video opens", bool(browse) and kept and saved and opened,
          f"{bool(ffs)} {bool(browse)} {kept} {saved} {opened} {good}")

    # high bit depth: the first 12 frames x4 plus random low bits as 10-bit FFV1; Preferences > Bit
    # depth 10 reloads the video at 10 bits
    import nv_depth as nd
    from PySide6.QtWidgets import QComboBox
    v10 = os.path.join(outdir, "src10.mkv")
    W, H = sinfo["W"], sinfo["H"]
    raw8 = subprocess.run(["ffmpeg", "-v", "error", "-i", video, "-frames:v", "12", "-f", "rawvideo",
                           "-pix_fmt", "yuv420p", "-"], capture_output=True, check=True).stdout
    a10 = np.frombuffer(raw8, np.uint8).astype("<u2") * 4 + \
        np.random.default_rng(10).integers(0, 4, len(raw8)).astype("<u2")
    tags = ["-colorspace", "bt709", "-color_range", "tv"]
    subprocess.run(["ffmpeg", "-y", "-v", "error", "-f", "rawvideo", "-pix_fmt", "yuv420p10le", "-s", f"{W}x{H}",
                    "-r", sinfo["fps"], *tags, "-i", "-", "-c:v", "ffv1", "-pix_fmt", "yuv420p10le", *tags, v10],
                   input=a10.tobytes(), check=True)
    win.open_video(v10)
    wait(app, lambda: win.src == v10 and st.profile.base is not None and idle())
    dlg = win.preferences()
    combo = [c for c in dlg.findChildren(QComboBox) if c.findData("10") >= 0][0]
    combo.setCurrentIndex(combo.findData("10"))
    dlg.accept()
    ok = wait(app, lambda: win.fmt is not None and win.fmt.bits == 10 and st.profile.base is not None and idle())
    f0 = win.frames[0] if win.frames else None
    check("10-bit: Bit depth 10 reloads as yuv420p10le float32 planes (0..255 scale)",
          ok and win.fmt.name == "yuv420p10le" and f0[0].dtype == np.float32 and f0[1].shape != f0[0].shape
          and float(f0[0].max()) <= 256.0, f"{win.fmt and win.fmt.name}")
    n0 = W * H + 2 * ((W + 1) // 2) * ((H + 1) // 2)
    check("10-bit: frame 0 = the 10-bit values / 4 exactly (low bits kept)",
          np.array_equal(np.concatenate([p.ravel() for p in f0]), a10[:n0].astype(np.float32) / 4)
          and np.mean(a10[:n0] % 4 != 0) > 0.5)
    lbl = win.depth_lbl.text()
    check("10-bit: status bar shows source pix_fmt and processing depth",
          "yuv420p10le (10-bit 4:2:0)" in lbl and "processing 10-bit" in lbl
          and "10-bit" in win.viewer_bar.size.text()
          and "Frame Bitdepth:&nbsp;&nbsp;</td><td>10-bit (yuv420p10le)" in win.filter_page.panel.perf.text(), lbl)
    win.tabs.setCurrentIndex(1)
    win.invalidate()
    win.schedule()
    check("10-bit: preview filtered (8-bit display)", wait(app, lambda: idle() and win.filt_cache and
                                                         win.viewer.filt is not None and win.viewer.filt.dtype == np.uint8))
    check("10-bit: export dialog defaults", win._export_filter()[1] == ".mp4")
    dst = os.path.join(outdir, "export10.mkv")
    lo, n = 4, 3
    win.process_video(dst, start=lo, frames=n)
    ok = wait(app, lambda: win.export is None, 1800)
    info = probe(dst)
    P = st.backend()
    prof = json.load(open(os.path.splitext(dst)[0] + ".ui_profile.json"))
    R = int(P["radius"])
    fmt = nd.pixfmt("yuv420p10le")
    got = [f for _, f in read_frames(dst, info["W"], info["H"], fps=info["fps"], fmt=fmt)]
    src = {i: f for i, f in read_frames(v10, info["W"], info["H"], lo - R, lo + n + R, info["fps"], fmt)}
    exact = ok and getattr(win, "export_ok", False) and info["pix_fmt"] == "yuv420p10le" and len(got) == n
    for k in range(min(n, len(got))):
        c = lo + k
        ids = [i for i in sorted(src) if abs(i - c) <= R]
        out = backend.process_frame([src[i] for i in ids], ids.index(c), prof, P)
        exp = nd.from_internal(out, fmt)
        exact &= exp == nd.from_internal(got[k], fmt)
    check("10-bit: export (FFV1 yuv420p10le) == backend.process_frame bit for bit", exact,
          f"{len(got)} frames, {info['pix_fmt']}")

    win.close()
    app.processEvents()
    n_fail = sum(1 for _, ok in RESULTS if not ok)
    print(f"{len(RESULTS) - n_fail}/{len(RESULTS)} passed")
    return 1 if n_fail else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1], sys.argv[2]))
