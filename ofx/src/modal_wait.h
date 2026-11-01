// modal_wait - the host's Cocoa event loop while a button of the plug-in waits for its window (N4d).
//
// The window is a separate process (session.h); InstanceChanged has to wait for its reply there, since OFX allows
// paramSetValue only inside that action.  A plain blocking read on the host's main thread stops the host taking
// events and the system flags it as not responding.  While waiting on the main thread of a host whose Cocoa
// application runs (NSApp non-nil and running; never created here):
//   - the thread sleeps in the kernel - poll() on the pipe (or a 10 ms poll while the request is written by a worker
//     thread, run()) - at most 10 ms at a time, and between the sleeps
//   - takes the host's queued events without waiting (nextEventMatchingMask: .. untilDate: distantPast) in a run loop
//     mode of its own, "MeatVideoOFXModal", so no timer, main-queue block or run loop source of the host fires (none is
//     scheduled in that mode); the queue is emptied, so no event is left unread for long;
//   - input events are kept consistent for the host: a mouse button / key pressed during the wait is discarded with
//     its release (the press beeps and brings the window's process to the front); a release whose press reached the
//     host before the wait is passed to -[NSApplication sendEvent:] (the host never sees a press without its release);
//     modifier changes (flagsChanged) are passed (the host's modifier state follows the keyboard); drags, moves,
//     scrolls, key repeats, tablet / gesture events and cursor updates are discarded;
//   - AppKit-defined events (window moved / exposed, screen changed, application activated / deactivated) are passed
//     to sendEvent: (AppKit's own bookkeeping); on the host's activation the window's process is brought back to the
//     front, as a modal panel keeps the focus;
//   - system-defined events (power button, media and volume keys) are discarded: hardware input;
//   - application-defined and periodic events are not taken (left in the queue for the host after the wait:
//     applications - Qt's event dispatcher among them - post them to wake their own loops).
// Otherwise (not the main thread, no running NSApp: a headless host such as ofx/tools/minihost) the waits are the
// plain blocking ones.  The Objective-C runtime is called from C++ (libobjc only); AppKit / Foundation are used only
// when the host has loaded them, so the plug-in does not load them into a host that has none.
#pragma once

#include <sys/types.h>

#include <cstdint>
#include <functional>
#include <set>

namespace mvofx {

class ModalWait {
public:
    // helper: the window's process (brought to the front while the host is active); 0 for none
    explicit ModalWait(pid_t helper);
    ~ModalWait();
    ModalWait(const ModalWait&) = delete;
    ModalWait& operator=(const ModalWait&) = delete;

    bool active() const { return active_; }   // the host's event loop is served
    // active: until fd is ready for `events` (POLLIN / POLLOUT) or has an error / hang-up, sleep in poll() (10 ms at a
    // time) and take the host's events in between; not active: return at once (the caller's call blocks)
    void wait_fd(int fd, short events);
    // waitpid of the child: blocking, or while active polled between the event drains; the status
    int wait_child(pid_t pid);
    // f() on a worker thread while the events are served (active), or here; returns when f has returned
    void run(const std::function<void()>& f);

private:
    void drain();                             // the host's queued events, without waiting
    void handle(void* ev);
    void front();                             // bring the window's process to the front (rate-limited)

    bool active_ = false;
    pid_t helper_ = 0;
    void* app_ = nullptr;                     // NSApp
    void* mode_ = nullptr;                    // NSString "MeatVideoOFXModal"
    void* past_ = nullptr;                    // [NSDate distantPast]
    double last_front_ = 0;
    uint32_t held_buttons_ = 0;               // mouse buttons pressed during the wait (their releases discarded too)
    std::set<unsigned short> held_keys_;      // keys pressed during the wait
};

}  // namespace mvofx
