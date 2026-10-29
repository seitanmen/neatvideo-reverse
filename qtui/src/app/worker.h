// worker - background work of the C++ UI, ported from repro/nv_ui_qt/worker.py (Task / TaskRunner): one task per
// slot; a request in a busy slot is queued and replaces an older queued one; done / failed run in the GUI thread.
#pragma once

#include <QObject>
#include <QString>
#include <QThread>

#include <any>
#include <atomic>
#include <functional>
#include <map>
#include <set>

namespace nvqt {

class Task : public QThread {
    Q_OBJECT
public:
    using Fn = std::function<std::any(Task&)>;
    Task(std::any tag, Fn fn, QObject* parent = nullptr) : QThread(parent), tag(std::move(tag)), fn(std::move(fn)) {}

    std::any tag;
    Fn fn;
    std::atomic<bool> cancelled{false};
    std::any result;
    QString error;
    bool ok = false;

    void report(double pct, const QString& text = QString()) { emit progress(pct, text); }
    void cancel() { cancelled = true; }

signals:
    void progress(double pct, QString text);

protected:
    void run() override;
};

class TaskRunner : public QObject {
    Q_OBJECT
public:
    using Done = std::function<void(const std::any& tag, std::any& result)>;
    using Fail = std::function<void(const std::any& tag, const QString& msg)>;
    using Progress = std::function<void(double, const QString&)>;

    explicit TaskRunner(QObject* parent = nullptr) : QObject(parent) {}
    ~TaskRunner() override;

    bool submit(const QString& slot, std::any tag, Task::Fn fn, Done on_done, Fail on_fail, Progress on_progress = {});
    bool busy(const QString& slot) const { return running_.count(slot) != 0; }
    bool idle() const { return running_.empty() && queued_.empty(); }
    void cancel(const QString& slot);
    void wait_all(int ms = 30000);

private:
    struct Queued {
        std::any tag;
        Task::Fn fn;
        Done done;
        Fail fail;
        Progress progress;
    };
    void finished(const QString& slot, Task* t, const Done& done, const Fail& fail);

    std::map<QString, Task*> running_;
    std::map<QString, Queued> queued_;
    std::set<Task*> all_;
};

}  // namespace nvqt
