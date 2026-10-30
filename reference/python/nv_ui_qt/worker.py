"""Background work: QThread tasks (preview, profiling, AFT, loading) and the export process."""
import os
import re
import signal
import sys
import traceback

from PySide6.QtCore import QObject, QProcess, QThread, Signal


class Task(QThread):
    """Runs fn(task) in a thread; fn may call task.report(pct, text) and check task.cancelled.
    done(tag, result) / failed(tag, message) are delivered in the GUI thread."""
    done = Signal(object, object)
    failed = Signal(object, str)
    progress = Signal(float, str)

    def __init__(self, tag, fn, parent=None):
        super().__init__(parent)
        self.tag, self.fn, self.cancelled = tag, fn, False

    def report(self, pct, text=""):
        self.progress.emit(float(pct), text)

    def cancel(self):
        self.cancelled = True

    def run(self):
        try:
            res = self.fn(self)
        except Exception as e:      # noqa: BLE001 - shown in the status bar
            if "Cancelled" not in type(e).__name__:      # Cancelled / ProfileCancelled are expected
                traceback.print_exc()
            self.failed.emit(self.tag, f"{type(e).__name__}: {e}")
            return
        self.done.emit(self.tag, res)


class TaskRunner(QObject):
    """Keeps references to running tasks; one task per slot (a new request in a busy slot is
    queued and replaces any older queued request)."""

    def __init__(self, parent=None):
        super().__init__(parent)
        self.running = {}
        self.queued = {}
        self.all = set()

    def submit(self, slot, tag, fn, on_done, on_fail, on_progress=None):
        if slot in self.running:
            self.queued[slot] = (tag, fn, on_done, on_fail, on_progress)
            return False
        t = Task(tag, fn)
        self.running[slot] = t
        self.all.add(t)
        t.done.connect(on_done)
        t.failed.connect(on_fail)
        if on_progress is not None:
            t.progress.connect(on_progress)
        t.finished.connect(lambda s=slot, t=t: self._finished(s, t))
        t.start()
        return True

    def busy(self, slot):
        return slot in self.running

    def cancel(self, slot):
        self.queued.pop(slot, None)
        if slot in self.running:
            self.running[slot].cancel()

    def _finished(self, slot, t):
        if self.running.get(slot) is t:
            del self.running[slot]
        self.all.discard(t)
        t.deleteLater()
        q = self.queued.pop(slot, None)
        if q is not None:
            self.submit(slot, *q)

    def wait_all(self, ms=30000):
        """Cancel everything and wait; tasks check `cancelled` (AFT runs in a child process that is
        terminated, profiling stops at the next FindFlatArea query, a preview frame finishes)."""
        self.queued.clear()
        for t in list(self.all):
            t.cancel()
        for t in list(self.all):
            t.wait(ms)


class ExportProcess(QObject):
    """python -m nv_ui_qt.export ...; parses denoise_video_v5's 'frame k/n' lines."""
    progress = Signal(float, str)
    finished = Signal(bool, str)

    def __init__(self, args, cwd, parent=None):
        super().__init__(parent)
        self.p = QProcess(self)
        self.p.setWorkingDirectory(cwd)
        self.p.setProcessChannelMode(QProcess.MergedChannels)
        self.p.readyReadStandardOutput.connect(self._read)
        self.p.finished.connect(self._done)
        self.args = [sys.executable, "-m", "nv_ui_qt.export", *args]
        self.log = []

    def start(self):
        self.p.start(self.args[0], self.args[1:])

    def kill(self):
        """Kill the export's whole process group (export.py makes itself the group leader), so the
        frame workers and ffmpeg do not outlive it; fall back to the process alone."""
        pid = int(self.p.processId() or 0)
        if pid > 0:
            try:
                if os.getpgid(pid) == pid:
                    os.killpg(pid, signal.SIGKILL)
                    return
            except OSError:
                pass
        self.p.kill()

    def _read(self):
        for line in bytes(self.p.readAllStandardOutput()).decode(errors="replace").splitlines():
            self.log.append(line)
            m = re.match(r"frame (\d+)/(\d+)", line)
            if m:
                self.progress.emit(100.0 * int(m.group(1)) / max(int(m.group(2)), 1), line)
            elif line.strip():
                self.progress.emit(-1.0, line)

    def _done(self, code, status):
        ok = status == QProcess.NormalExit and code == 0
        tail = "\n".join(self.log[-6:])
        self.finished.emit(ok, tail if not ok else (self.log[-1] if self.log else ""))
