"""Menu bar as the genuine 6.1.3 Pro shows it in Resolve: Effect / View / Profile / Filter / Variants / Tools /
Help, with the Beginner / Advanced Mode switch in the right corner. Effect, Profile and Help list the
genuine items in the genuine order (observed, sandbox/omo/coord/genuine_ui/OBSERVED.md); items without a
counterpart in the reproduction are disabled, reproduction-only items follow a separator. Undo / Redo
(one undo stack for profile and filter changes) are in the Profile and Filter menus. Variants has no
counterpart (disabled)."""
import shiboken6
from PySide6.QtGui import QAction, QActionGroup, QKeySequence
from PySide6.QtWidgets import QMenu, QToolButton

from .widgets.viewer import MODE_SECTIONS, MODES, SPLITS, ZOOMS

N_A = "Not available in the reproduction"


def build_menus(win):
    mb = win.menuBar()
    A = {}

    def add(menu, key, text, slot, shortcut=None, enabled=True, tip=None, checkable=False):
        a = QAction(text, win)
        if shortcut:
            a.setShortcut(QKeySequence(shortcut))
        if tip:
            a.setStatusTip(tip)
            a.setToolTip(tip)
        a.setEnabled(enabled)
        a.setCheckable(checkable)
        if checkable:
            a.toggled.connect(slot)
        else:
            a.triggered.connect(lambda checked=False, slot=slot: slot())
        menu.addAction(a)
        A[key] = a
        return a

    def na(menu, text, shortcut=None):
        a = menu.addAction(text)
        a.setEnabled(False)
        a.setToolTip(N_A)
        a.setStatusTip(N_A)
        if shortcut:
            a.setText(f"{text}\t{shortcut}")
        return a

    m = mb.addMenu("E&ffect")
    add(m, "copy_both", "Copy Profile and Preset", win.copy_both, "Shift+Ctrl+C")
    add(m, "paste_both", "Paste Profile and Preset", win.paste_both, "Shift+Ctrl+V")
    na(m, "Skip Neat Video Processing")
    # "\tReturn" shows the genuine's shortcut without taking the Return key from the side panels
    add(m, "apply", "Apply\tReturn", lambda: win.b_apply.click(), tip=win.b_apply.toolTip())
    add(m, "cancel", "Cancel", lambda: win.b_cancel.click(), tip="Close the window (the genuine plug-in "
        "returns to the host)")
    m.addSeparator()
    m.addSection("Reproduction")
    add(m, "open", "Open Input Video…", win.open_video, QKeySequence.Open, tip="repro: load a video file")
    add(m, "save_frame", "Save Output Image As…", win.save_frame, tip="Save the primary viewer pane as PNG")
    add(m, "export", "Export Full Video...", win.process_video, "Ctrl+E", tip="Process the full video with the current settings")

    u = win.state.undo.createUndoAction(win, "Undo Last Change")
    u.setShortcut(QKeySequence.Undo)
    r = win.state.undo.createRedoAction(win, "Redo Last Change")
    r.setShortcuts([QKeySequence("Shift+Ctrl+Z"), QKeySequence("Ctrl+Y")])
    # genuine labels stay "Undo / Redo Last Change"; the step name goes to the tooltip
    # (the stack can outlive the actions at shutdown: skip deleted ones)
    def relabel(a, text):
        return lambda t: shiboken6.isValid(a) and (a.setText(text), a.setToolTip(t))
    win.state.undo.undoTextChanged.connect(relabel(u, "Undo Last Change"))
    win.state.undo.redoTextChanged.connect(relabel(r, "Redo Last Change"))
    A["undo"], A["redo"] = u, r

    m = mb.addMenu("&View")
    z = m.addMenu("&Zoom")
    add(z, "zoom_fit", "Zoom to Fit", lambda: win.viewer.set_zoom("fit"), "Ctrl+0")
    add(z, "zoom_in", "Zoom In", lambda: win.viewer.zoom_step(1), "Ctrl++")
    add(z, "zoom_out", "Zoom Out", lambda: win.viewer.zoom_step(-1), "Ctrl+-")
    z.addSeparator()
    for p in ZOOMS:
        add(z, f"zoom_{p:g}", f"{p:g}%", lambda p=p: win.viewer.set_zoom(p), "Ctrl+Alt+0" if p == 100 else None)
    vm = m.addMenu("&Viewer Mode")
    g = QActionGroup(win)
    for i, (short, long) in enumerate(MODES):
        if i in MODE_SECTIONS:
            vm.addSection(MODE_SECTIONS[i])
        a = add(vm, f"mode_{short}", long, lambda short=short: win.viewer.set_mode(short), f"Ctrl+{i + 1}")
        a.setCheckable(True)
        g.addAction(a)
    vm.addSection("Viewer Split")
    g = QActionGroup(win)
    for s in SPLITS:
        a = add(vm, f"split_{s}", s, lambda s=s: win.viewer.set_split(s))
        a.setCheckable(True)
        g.addAction(a)
    add(vm, "split_next", "Next Split Mode", lambda: win.cycle_split(1), "Ctrl+]")
    add(vm, "split_prev", "Previous Split Mode", lambda: win.cycle_split(-1), "Ctrl+[")
    m.addSeparator()
    add(m, "show_preview", "Show Preview", lambda: win.preview_btn.toggle(), "Alt+H", tip="Toggle Auto Preview")
    add(m, "pan", "Toggle Panning Mode", lambda: win.viewer_bar.pan.toggle(), "Ctrl+D")
    add(m, "play", "Start Playback", lambda: win.play_btn.toggle(), "Ctrl+P")

    m = mb.addMenu("&Profile")
    m.addAction(u)
    m.addAction(r)
    m.addSeparator()
    add(m, "p_load", "&Load Profile…", win.load_profile)
    add(m, "p_save", "&Save As…", win.save_profile)
    add(m, "p_reset", "&Reset Profile", win.reset_profile)
    m.addSeparator()
    add(m, "p_copy", "Copy Profile", win.copy_profile, QKeySequence.Copy)
    add(m, "p_paste", "Paste Profile", win.paste_profile, QKeySequence.Paste)
    m.addSeparator()
    na(m, "Save As Default Profile…")
    add(m, "p_assign", "Assign As Default Profile", win.assign_default_profile)
    add(m, "p_reset_default", "Reset Default Profile", win.reset_default_profile)
    add(m, "p_change_default", "Change Default Profile…", win.change_default_profile)
    m.addSeparator()
    add(m, "p_auto_noisy", "Auto Profile (Most Noisy Frame)", lambda: win.auto_profile(True), "F2")
    add(m, "p_auto", "Auto Profile (Current Frame)", lambda: win.auto_profile(False), "Shift+F2")
    add(m, "p_generic", "Generic Profile", win.generic_profile, "F6")
    m.addSeparator()
    add(m, "p_viewer", "Profile Viewer…", win.profile_viewer, "Ctrl+Shift+I",
        tip="Profile viewer: details of current noise profile")
    m.addSeparator()
    m.addSection("Reproduction")
    add(m, "p_build_noisy", "Build Profile (Most Noisy Frame)", lambda: win.build_profile(True))
    add(m, "p_build", "Build Profile (Current Frame)", lambda: win.build_profile(False))
    add(m, "p_aft", "Auto Fine-Tune", win.auto_fine_tune, "Ctrl+F")
    add(m, "p_complete", "Auto Com&plete", win.auto_complete)
    add(m, "p_reset_ft", "R&eset Fine-Tuning Results", win.reset_fine_tune)
    add(m, "p_check", "Profile Check", win.set_profile_check, "Ctrl+K", checkable=True,
        tip="Profile Check: Assist Image preview to find the optimal Noise Level (same toggle as Begin)")
    add(m, "p_match", "Auto Match (n/a)", lambda: None, "Ctrl+M", enabled=False, tip="Not available in the reproduction")
    add(m, "p_converter", "Profile Converter…", lambda: None, enabled=False,
        tip="Not available: genuine .dnp profiles are not read or written by the reproduction")

    m = mb.addMenu("F&ilter")
    m.addAction(u)
    m.addAction(r)
    m.addSeparator()
    add(m, "f_load", "&Load Filter Preset…", win.load_preset)
    add(m, "f_save", "&Save As Filter Preset…", win.save_preset)
    add(m, "f_reset", "&Reset Filter Settings", win.reset_filter)
    add(m, "f_copy", "Copy Filter Settings", win.copy_filter)
    add(m, "f_paste", "Paste Filter Settings", win.paste_filter)
    m.addSeparator()
    add(m, "f_assign", "Assign As Default Preset", win.assign_default_preset)
    add(m, "f_reset_default", "Reset Default Preset", win.reset_default_preset)
    add(m, "f_change_default", "Change Default Preset…", win.change_default_preset)
    add(m, "f_desc", "Preset &Description…", win.preset_description)
    m.addSeparator()
    add(m, "f_assist", "F&ilter Tuning Assist", win.set_assist, checkable=True)
    add(m, "f_check", "Filter Settings &Check", lambda: win.guide("filter"))

    m = mb.addMenu("V&ariants")
    na(m, "Variants (alternative filter settings)")

    m = mb.addMenu("T&ools")
    add(m, "batch", "&Batch Profiler…", win.batch_profiler)
    m.addSeparator()
    add(m, "prefs", "Preferences…", win.preferences, "F10")
    add(m, "perf", "Performance tab", win.performance_tab, "Shift+Ctrl+P")
    m.addSeparator()
    add(m, "nlc", "Noise Level Check", win.set_noise_level_check, checkable=True,
        tip="Noise Level Check: Temporal Filter mask in the viewer (same toggle as Begin on the Temporal tab "
            "and Filter Settings Check > 1. Noise Level Check)")

    m = mb.addMenu("&Help")
    add(m, "help", "Help", lambda: win.guide("tuning"), "F1", tip="repro: shows the Quick Start Guide text")
    na(m, "Read Me")
    na(m, "What's New")
    add(m, "qsg", "&Quick Start Guide", lambda: win.guide("tuning"))
    na(m, "How to Use Neat Video")
    m.addSeparator()
    na(m, "Web Page")
    na(m, "Community Forum")
    na(m, "Report a Bug Online")
    na(m, "Support via E-mail")
    m.addSeparator()
    add(m, "about", "About…", win.about, "Alt+F1")

    mode = QToolButton()
    mode.setPopupMode(QToolButton.InstantPopup)
    mode.setAutoRaise(True)
    mode.setStyleSheet("QToolButton{font-weight:bold; padding:2px 8px}")
    mm = QMenu(mode)
    g = QActionGroup(win)
    for key, text, adv in (("beginner", "&Beginner Mode", False), ("advanced", "&Advanced Mode", True)):
        a = QAction(text, win)
        a.setCheckable(True)
        a.triggered.connect(lambda checked=False, adv=adv: win.set_advanced(adv))
        g.addAction(a)
        mm.addAction(a)
        A[key] = a
    mode.setMenu(mm)
    mb.setCornerWidget(mode)
    A["mode_button"] = mode
    return A
