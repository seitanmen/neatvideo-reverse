"""Menu bar: File (repro) / Edit / Profile / Filter / Tools / Help (UI_SPEC.md §4)."""
from PySide6.QtGui import QAction, QKeySequence


def build_menus(win):
    mb = win.menuBar()
    A = {}

    def add(menu, key, text, slot, shortcut=None, enabled=True, tip=None):
        a = QAction(text, win)
        if shortcut:
            a.setShortcut(QKeySequence(shortcut))
        if tip:
            a.setStatusTip(tip)
            a.setToolTip(tip)
        a.setEnabled(enabled)
        a.triggered.connect(lambda checked=False, slot=slot: slot())
        menu.addAction(a)
        A[key] = a
        return a

    m = mb.addMenu("&File")
    add(m, "open", "Open Video…", win.open_video, QKeySequence.Open)
    add(m, "export", "Process Full Video…", win.process_video, "Ctrl+E")
    add(m, "save_frame", "Save Preview Frame As PNG…", win.save_frame)
    m.addSeparator()
    add(m, "quit", "Quit", win.close, QKeySequence.Quit)

    m = mb.addMenu("&Edit")
    u = win.state.undo.createUndoAction(win, "Undo")
    u.setShortcut(QKeySequence.Undo)
    r = win.state.undo.createRedoAction(win, "Redo")
    r.setShortcut(QKeySequence.Redo)
    m.addAction(u)
    m.addAction(r)
    A["undo"], A["redo"] = u, r

    m = mb.addMenu("&Profile")
    add(m, "p_load", "Load Profile…", win.load_profile)
    add(m, "p_save", "Save Device Noise Profile As…", win.save_profile)
    add(m, "p_reset", "Reset Profile", win.reset_profile)
    add(m, "p_change_default", "Change Default Profile…", win.change_default_profile)
    m.addSeparator()
    add(m, "p_copy", "Copy Profile", win.copy_profile)
    add(m, "p_paste", "Paste Profile", win.paste_profile)
    add(m, "p_copy_from", "Copy Noise Profile from…", win.load_profile,
        tip="Load the profile saved by another session (same as Load Profile)")
    add(m, "p_assign", "Assign As Default Profile", win.assign_default_profile)
    m.addSeparator()
    add(m, "p_viewer", "Profile Viewer…", win.profile_check)
    add(m, "p_converter", "Profile Converter…", lambda: None, enabled=False,
        tip="Not available: genuine .dnp profiles are not read or written by the reproduction")

    m = mb.addMenu("F&ilter")
    add(m, "f_load", "Load Filter Preset…", win.load_preset)
    add(m, "f_save", "Save Filter Preset As…", win.save_preset)
    add(m, "f_reset", "Reset Filter Settings", win.reset_filter)
    add(m, "f_change_default", "Change Default Preset…", win.change_default_preset)
    m.addSeparator()
    add(m, "f_copy", "Copy Filter Settings", win.copy_filter)
    add(m, "f_paste", "Paste Filter Settings", win.paste_filter)
    add(m, "f_assign", "Assign As Default Preset", win.assign_default_preset)

    m = mb.addMenu("&Tools")
    add(m, "prefs", "Preferences…", win.preferences)
    add(m, "batch", "Batch Profiler…", win.batch_profiler)
    m.addSeparator()
    add(m, "g_filter", "Filter Settings Check", lambda: win.guide("filter"))
    add(m, "g_noise", "Noise Level Check", lambda: win.guide("noise"))
    add(m, "g_profile", "Profile Check", win.profile_check)

    m = mb.addMenu("&Help")
    add(m, "about", "About", win.about)
    return A
