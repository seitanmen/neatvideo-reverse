# wait (event driven, kqueue NOTE_EXIT; no polling) until process PID exits or TIMEOUT seconds pass
import select, sys
pid, timeout = int(sys.argv[1]), float(sys.argv[2])
kq = select.kqueue()
try:
    ev = select.kevent(pid, filter=select.KQ_FILTER_PROC, flags=select.KQ_EV_ADD | select.KQ_EV_ONESHOT,
                       fflags=select.KQ_NOTE_EXIT)
    r = kq.control([ev], 1, timeout)
except ProcessLookupError:
    print("already exited"); sys.exit(0)
print("exited" if r else "still running")
