// modal_wait - see modal_wait.h.
#include "modal_wait.h"

#include <dlfcn.h>
#include <objc/message.h>
#include <objc/runtime.h>
#include <poll.h>
#include <pthread.h>
#include <sys/wait.h>

#include <atomic>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <thread>

extern "C" void* objc_autoreleasePoolPush(void);
extern "C" void objc_autoreleasePoolPop(void* pool);

namespace mvofx {

namespace {

using Id = id;

SEL sel(const char* s) { return sel_registerName(s); }

// objc_msgSend with the signature of each call
Id msg(Id o, const char* s) { return reinterpret_cast<Id (*)(Id, SEL)>(objc_msgSend)(o, sel(s)); }
bool msg_bool(Id o, const char* s) { return reinterpret_cast<BOOL (*)(Id, SEL)>(objc_msgSend)(o, sel(s)) != NO; }
void send_event(Id app, Id ev) { reinterpret_cast<void (*)(Id, SEL, Id)>(objc_msgSend)(app, sel("sendEvent:"), ev); }

double now() {
    timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return static_cast<double>(ts.tv_sec) + 1e-9 * static_cast<double>(ts.tv_nsec);
}

// NSEventType values (AppKit NSEvent.h)
enum : unsigned long {
    LeftMouseDown = 1, LeftMouseUp = 2, RightMouseDown = 3, RightMouseUp = 4, KeyDown = 10, KeyUp = 11,
    FlagsChanged = 12, AppKitDefined = 13, ApplicationDefined = 15, Periodic = 16, OtherMouseDown = 25,
    OtherMouseUp = 26
};
constexpr short ApplicationActivated = 1;   // NSEventSubtypeApplicationActivated
// every event but the application-defined and periodic ones (those stay in the queue)
constexpr unsigned long long MASK = ~((1ull << ApplicationDefined) | (1ull << Periodic));
constexpr int SLEEP_MS = 10;                // the longest the host's events wait unread
constexpr int DRAIN_MAX = 256;              // events taken per drain at most (the sleep follows)

// MV_MODAL_WAIT=0 builds the plain blocking wait only (the control of ofx/tools/verify_modal.py)
#ifndef MV_MODAL_WAIT
#define MV_MODAL_WAIT 1
#endif

}  // namespace

ModalWait::ModalWait(pid_t helper) : helper_(helper) {
    // NSApp as the host made it: AppKit's global, looked up (never sharedApplication: that would create one)
    Id* nsapp = MV_MODAL_WAIT && pthread_main_np() ? static_cast<Id*>(dlsym(RTLD_DEFAULT, "NSApp")) : nullptr;
    Id str = reinterpret_cast<Id>(objc_getClass("NSString"));
    Id date = reinterpret_cast<Id>(objc_getClass("NSDate"));
    if (nsapp && *nsapp && str && date && msg_bool(*nsapp, "isRunning")) {
        app_ = *nsapp;
        mode_ = msg(reinterpret_cast<Id (*)(Id, SEL, const char*)>(objc_msgSend)(str, sel("stringWithUTF8String:"),
                                                                                "MeatVideoOFXModal"),
                    "retain");
        past_ = msg(msg(date, "distantPast"), "retain");
        active_ = mode_ && past_;
    }
    // (tests) which wait a session takes
    if (std::getenv("MEATVIDEO_OFX_TRACE"))
        std::fprintf(stderr, "meatvideo-ofx: helper wait: %s\n",
                     active_ ? "host event loop (MeatVideoOFXModal)" : "blocking");
}

ModalWait::~ModalWait() {
    if (mode_) msg(static_cast<Id>(mode_), "release");
    if (past_) msg(static_cast<Id>(past_), "release");
}

void ModalWait::front() {
    if (!helper_) return;
    const double t = now();
    if (t - last_front_ < 0.25) return;
    last_front_ = t;
    Id ra_cls = reinterpret_cast<Id>(objc_getClass("NSRunningApplication"));
    if (!ra_cls) return;
    Id ra = reinterpret_cast<Id (*)(Id, SEL, pid_t)>(objc_msgSend)(ra_cls, sel("runningApplicationWithProcessIdentifier:"),
                                                                 helper_);
    if (!ra || msg_bool(ra, "isActive")) return;
    // cooperative activation (macOS 14+): the active host yields to the window's process, which then activates
    Id app = static_cast<Id>(app_);
    if (msg_bool(app, "isActive"))
        reinterpret_cast<void (*)(Id, SEL, Id)>(objc_msgSend)(app, sel("yieldActivationToApplication:"), ra);
    reinterpret_cast<BOOL (*)(Id, SEL, unsigned long)>(objc_msgSend)(ra, sel("activateWithOptions:"), 1ul /* all windows */);
}

void ModalWait::handle(void* evp) {
    Id ev = static_cast<Id>(evp);
    Id app = static_cast<Id>(app_);
    const unsigned long type = reinterpret_cast<unsigned long (*)(Id, SEL)>(objc_msgSend)(ev, sel("type"));
    auto refuse = [&] {
        if (auto beep = reinterpret_cast<void (*)()>(dlsym(RTLD_DEFAULT, "NSBeep"))) beep();
        front();
    };
    switch (type) {
    case AppKitDefined:
        send_event(app, ev);
        if (reinterpret_cast<short (*)(Id, SEL)>(objc_msgSend)(ev, sel("subtype")) == ApplicationActivated) front();
        return;
    case LeftMouseDown:
    case RightMouseDown:
    case OtherMouseDown: {
        const long b = reinterpret_cast<long (*)(Id, SEL)>(objc_msgSend)(ev, sel("buttonNumber"));
        if (b >= 0 && b < 32) held_buttons_ |= 1u << b;
        refuse();
        return;
    }
    case LeftMouseUp:
    case RightMouseUp:
    case OtherMouseUp: {
        const long b = reinterpret_cast<long (*)(Id, SEL)>(objc_msgSend)(ev, sel("buttonNumber"));
        const uint32_t bit = b >= 0 && b < 32 ? 1u << b : 0;
        if (held_buttons_ & bit)
            held_buttons_ &= ~bit;            // pressed during the wait: both discarded
        else
            send_event(app, ev);              // pressed before the wait: the host gets its release
        return;
    }
    case KeyDown: {
        const unsigned short k = reinterpret_cast<unsigned short (*)(Id, SEL)>(objc_msgSend)(ev, sel("keyCode"));
        if (!msg_bool(ev, "isARepeat")) {
            if (!held_keys_.count(k)) refuse();
            held_keys_.insert(k);
        }
        return;                               // a repeat of a key pressed before the wait: discarded too
    }
    case KeyUp: {
        const unsigned short k = reinterpret_cast<unsigned short (*)(Id, SEL)>(objc_msgSend)(ev, sel("keyCode"));
        if (!held_keys_.erase(k)) send_event(app, ev);
        return;
    }
    case FlagsChanged:
        send_event(app, ev);                  // the modifier state as the keyboard has it
        return;
    default:
        return;                               // moves, drags, scroll, tablet, gestures, cursor updates, system-defined
    }
}

void ModalWait::drain() {
    Id app = static_cast<Id>(app_);
    for (int k = 0; k < DRAIN_MAX; ++k) {
        void* pool = objc_autoreleasePoolPush();
        Id ev = reinterpret_cast<Id (*)(Id, SEL, unsigned long long, Id, Id, BOOL)>(objc_msgSend)(
            app, sel("nextEventMatchingMask:untilDate:inMode:dequeue:"), MASK, static_cast<Id>(past_),
            static_cast<Id>(mode_), YES);
        if (ev) handle(ev);
        objc_autoreleasePoolPop(pool);
        if (!ev) break;
    }
    if (msg_bool(app, "isActive")) front();   // the host is in front: the window's process belongs there
}

void ModalWait::wait_fd(int fd, short events) {
    if (!active_) return;
    for (;;) {
        drain();
        pollfd p = {fd, events, 0};
        const int r = ::poll(&p, 1, SLEEP_MS);           // the thread sleeps in the kernel
        if (r > 0 || (r < 0 && errno != EINTR)) return;   // ready, hung up, error (the caller's read / write tells)
    }
}

int ModalWait::wait_child(pid_t pid) {
    int status = 0;
    if (!active_) {
        while (::waitpid(pid, &status, 0) < 0 && errno == EINTR) {
        }
        return status;
    }
    for (;;) {
        const pid_t r = ::waitpid(pid, &status, WNOHANG);
        if (r == pid || (r < 0 && errno != EINTR)) return status;
        drain();
        ::poll(nullptr, 0, SLEEP_MS);
    }
}

void ModalWait::run(const std::function<void()>& f) {
    if (!active_) {
        f();
        return;
    }
    std::atomic<bool> done{false};
    std::thread worker([&] {
        f();
        done = true;
    });
    while (!done) {
        drain();
        ::poll(nullptr, 0, SLEEP_MS);
    }
    worker.join();
}

}  // namespace mvofx
