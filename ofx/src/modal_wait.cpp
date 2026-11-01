// modal_wait - see modal_wait.h.
#include "modal_wait.h"

#include <dlfcn.h>
#include <objc/message.h>
#include <objc/runtime.h>
#include <poll.h>
#include <pthread.h>
#include <sys/wait.h>

#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <ctime>

extern "C" void* objc_autoreleasePoolPush(void);
extern "C" void objc_autoreleasePoolPop(void* pool);

namespace mvofx {

namespace {

using Id = id;

SEL sel(const char* s) { return sel_registerName(s); }

// objc_msgSend with the signature of each call
Id msg(Id o, const char* s) { return reinterpret_cast<Id (*)(Id, SEL)>(objc_msgSend)(o, sel(s)); }
bool msg_bool(Id o, const char* s) {
    return reinterpret_cast<BOOL (*)(Id, SEL)>(objc_msgSend)(o, sel(s)) != NO;
}

double now() {
    timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return static_cast<double>(ts.tv_sec) + 1e-9 * static_cast<double>(ts.tv_nsec);
}

// NSEventType values (AppKit NSEvent.h)
enum : unsigned long {
    LeftMouseDown = 1, RightMouseDown = 3, KeyDown = 10, AppKitDefined = 13, SystemDefined = 14,
    ApplicationDefined = 15, Periodic = 16, OtherMouseDown = 25
};
constexpr short ApplicationActivated = 1;   // NSEventSubtypeApplicationActivated
// every event but the application-defined and periodic ones (those stay in the queue)
constexpr unsigned long long MASK = ~((1ull << ApplicationDefined) | (1ull << Periodic));

// MV_MODAL_WAIT=0 builds the plain blocking wait only (the control of ofx/tools/cocoahost)
#ifndef MV_MODAL_WAIT
#define MV_MODAL_WAIT 1
#endif

}  // namespace

ModalWait::ModalWait(pid_t helper) : helper_(helper) {
    // NSApp as the host made it: AppKit's global, looked up (never sharedApplication: that would create one)
    Id* nsapp = MV_MODAL_WAIT && pthread_main_np() ? static_cast<Id*>(dlsym(RTLD_DEFAULT, "NSApp")) : nullptr;
    Id cls = reinterpret_cast<Id>(objc_getClass("NSString"));
    if (nsapp && *nsapp && cls && msg_bool(*nsapp, "isRunning")) {
        app_ = *nsapp;
        mode_ = msg(reinterpret_cast<Id (*)(Id, SEL, const char*)>(objc_msgSend)(cls, sel("stringWithUTF8String:"),
                                                                                "MeatVideoOFXModal"),
                    "retain");
        active_ = mode_ != nullptr;
    }
    // (tests) which wait a session takes
    if (std::getenv("MEATVIDEO_OFX_TRACE"))
        std::fprintf(stderr, "meatvideo-ofx: helper wait: %s\n",
                     active_ ? "host event loop (MeatVideoOFXModal)" : "blocking");
}

ModalWait::~ModalWait() {
    if (mode_) msg(static_cast<Id>(mode_), "release");
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

void ModalWait::pump(double seconds) {
    void* pool = objc_autoreleasePoolPush();
    Id app = static_cast<Id>(app_);
    Id date_cls = reinterpret_cast<Id>(objc_getClass("NSDate"));
    Id until = reinterpret_cast<Id (*)(Id, SEL, double)>(objc_msgSend)(date_cls, sel("dateWithTimeIntervalSinceNow:"), seconds);
    Id ev = reinterpret_cast<Id (*)(Id, SEL, unsigned long long, Id, Id, BOOL)>(objc_msgSend)(
        app, sel("nextEventMatchingMask:untilDate:inMode:dequeue:"), MASK, until, static_cast<Id>(mode_), YES);
    if (ev) {
        const unsigned long type = reinterpret_cast<unsigned long (*)(Id, SEL)>(objc_msgSend)(ev, sel("type"));
        if (type == AppKitDefined) {
            reinterpret_cast<void (*)(Id, SEL, Id)>(objc_msgSend)(app, sel("sendEvent:"), ev);
            const short st = reinterpret_cast<short (*)(Id, SEL)>(objc_msgSend)(ev, sel("subtype"));
            if (st == ApplicationActivated) front();
        } else if (type == LeftMouseDown || type == RightMouseDown || type == OtherMouseDown || type == KeyDown) {
            if (auto beep = reinterpret_cast<void (*)()>(dlsym(RTLD_DEFAULT, "NSBeep"))) beep();
            front();
        }
        // anything else (input, system-defined): discarded
    } else if (msg_bool(app, "isActive")) {
        front();                              // the host is in front: the window's process belongs there
    }
    objc_autoreleasePoolPop(pool);
}

void ModalWait::wait_fd(int fd, short events) {
    if (!active_) return;
    for (;;) {
        pollfd p = {fd, events, 0};
        const int r = ::poll(&p, 1, 0);
        if (r > 0 || (r < 0 && errno != EINTR)) return;   // ready, hung up, error (the caller's read / write tells)
        pump(0.02);
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
        pump(0.02);
    }
}

}  // namespace mvofx
