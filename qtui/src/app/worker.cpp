// worker - see worker.h.
#include "worker.h"

#include <exception>

namespace nvqt {

void Task::run() {
    try {
        result = fn(*this);
        ok = true;
    } catch (const std::exception& e) {
        error = QString::fromStdString(e.what());   // "Cancelled: ..." / "ProfileCancelled: ..." / the message
    } catch (...) {
        error = "unknown error";
    }
}

TaskRunner::~TaskRunner() { wait_all(); }

bool TaskRunner::submit(const QString& slot, std::any tag, Task::Fn fn, Done on_done, Fail on_fail, Progress on_progress) {
    if (running_.count(slot)) {
        queued_[slot] = Queued{std::move(tag), std::move(fn), std::move(on_done), std::move(on_fail), std::move(on_progress)};
        return false;
    }
    auto* t = new Task(std::move(tag), std::move(fn));
    running_[slot] = t;
    all_.insert(t);
    if (on_progress)
        connect(t, &Task::progress, this, [on_progress](double pct, QString text) { on_progress(pct, text); });
    connect(t, &QThread::finished, this, [this, slot, t, on_done, on_fail]() { finished(slot, t, on_done, on_fail); });
    t->start();
    return true;
}

void TaskRunner::cancel(const QString& slot) {
    queued_.erase(slot);
    auto it = running_.find(slot);
    if (it != running_.end()) it->second->cancel();
}

void TaskRunner::finished(const QString& slot, Task* t, const Done& done, const Fail& fail) {
    // Python: Task.done / failed (emitted before QThread.finished) first, then TaskRunner._finished
    if (t->ok) {
        if (done) done(t->tag, t->result);
    } else if (fail) {
        fail(t->tag, t->error);
    }
    auto it = running_.find(slot);
    if (it != running_.end() && it->second == t) running_.erase(it);
    all_.erase(t);
    t->deleteLater();
    auto q = queued_.find(slot);
    if (q != queued_.end()) {
        Queued n = std::move(q->second);
        queued_.erase(q);
        submit(slot, std::move(n.tag), std::move(n.fn), std::move(n.done), std::move(n.fail), std::move(n.progress));
    }
}

void TaskRunner::wait_all(int ms) {
    queued_.clear();
    for (Task* t : all_) t->cancel();
    for (Task* t : all_) t->wait(static_cast<unsigned long>(ms));
}

}  // namespace nvqt
