// modal_wait - the host's Cocoa event loop while a button of the plug-in waits for its window (N4d).
//
// The window is a separate process (session.h); InstanceChanged has to wait for its reply there, since OFX allows
// paramSetValue only inside that action.  A plain blocking read on the host's main thread stops the host taking
// events and the system flags it as not responding.  While waiting on the main thread of a host whose Cocoa
// application runs (NSApp non-nil and running; never created here), the wait instead fetches the host's events in a
// run loop mode of its own, "MeatVideoOFXModal" - so no timer, main-queue block or run loop source of the host fires
// (they are not scheduled in that mode) - and:
//   - input events (mouse, keys, scroll, tablet, gestures, cursor updates) are discarded: the host gets no input while
//     the window is open (a click or a key press beeps and brings the window's process to the front);
//   - AppKit-defined events (window moved / exposed, screen changed, application activated / deactivated) are passed
//     to -[NSApplication sendEvent:]: AppKit's own bookkeeping of the windows and the activation state; on the host's
//     activation the window's process is brought back to the front, as a modal panel keeps the focus;
//   - system-defined events (power button, media and volume keys) are discarded: hardware input;
//   - application-defined and periodic events are not fetched at all (left in the queue for the host after the wait:
//     applications - Qt's event dispatcher among them - post them to wake their own loops).
// Otherwise (not the main thread, no running NSApp: a headless host such as ofx/tools/minihost) the wait is the
// plain blocking one.  The Objective-C runtime is called from C++ (libobjc only); AppKit / Foundation are used only
// when the host has loaded them, so the plug-in does not load them into a host that has none.
#pragma once

#include <sys/types.h>

namespace mvofx {

class ModalWait {
public:
    // helper: the window's process (brought to the front while the host is active); 0 for none
    explicit ModalWait(pid_t helper);
    ~ModalWait();
    ModalWait(const ModalWait&) = delete;
    ModalWait& operator=(const ModalWait&) = delete;

    bool active() const { return active_; }   // the host's event loop is pumped
    // active: pump the host's events until fd is ready for `events` (POLLIN / POLLOUT) or has an error / hang-up
    void wait_fd(int fd, short events);
    // waitpid of the child: blocking, or while active polled between pumps; the status
    int wait_child(pid_t pid);

private:
    void pump(double seconds);                // fetch and handle the host's events for up to `seconds`
    void front();                             // bring the window's process to the front (rate-limited)

    bool active_ = false;
    pid_t helper_ = 0;
    void* app_ = nullptr;                     // NSApp
    void* mode_ = nullptr;                    // NSString "MeatVideoOFXModal"
    double last_front_ = 0;
};

}  // namespace mvofx
