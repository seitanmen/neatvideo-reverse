// cocoahost - the test host of ofx/tools/minihost.cpp inside a running Cocoa application (N4d: the host stays
// responsive while a button's window is open, ofx/src/modal_wait.h).
//
//   cocoahost [--poke | --destroy] [--held] [--activate] BUNDLE <minihost arguments>
//
// An NSApplication (activation policy regular, never activated: it does not take the focus) with one small window
// runs its event loop; 0.5 s in, a default-mode timer runs the minihost session (the arguments as minihost takes
// them: --frames .. --press prepare@T ..) on the main thread, so the plug-in's waits happen on the main thread of a
// running NSApp, as in a real host.  Meanwhile the host records on stderr, with CLOCK_REALTIME times:
//   "tick <t>"       every fire of a repeating default-mode timer (0.1 s)
//   "input <type> <t>" every input event the application dispatches (a local event monitor: called from sendEvent:)
//   "block <t>"      a main-queue block (dispatch_after 2 s into the session) that calls the plug-in (InstanceChanged
//                    of ClipParams) - it must not run inside the plug-in's wait
// With --activate the application is activated (in front, its window key) as a host is when its button is pressed;
// with --held a left mouse button and the key "s" are pressed (dispatched) before the wait, for releases during it;
// with --destroy the background thread calls DestroyInstance instead (the plug-in must free the instance only after
// its wait); with --poke a background thread presses Prepare Noise Profile again 3 s into the session (the plug-in must
// refuse a second window).  "cocoahost: pid <pid> ready" is printed when the loop runs; "cocoahost: session done"
// after the minihost session (its log: --log FILE); then the application terminates.
#import <AppKit/AppKit.h>

#include <pthread.h>

#define MINIHOST_EMBEDDED 1
#include "minihost.cpp"

namespace {

double now_wall() {
    timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return static_cast<double>(ts.tv_sec) + 1e-9 * static_cast<double>(ts.tv_nsec);
}

void say(const char* fmt, ...) __attribute__((format(printf, 1, 2)));
void say(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    std::vfprintf(stderr, fmt, ap);
    va_end(ap);
    std::fflush(stderr);
}

}  // namespace

int main(int argc, char** argv) {
    @autoreleasepool {
        bool poke = false, activate = false;
        static bool destroy = false;
        bool held = false;
        std::vector<char*> av(argv, argv + argc);
        while (av.size() > 1 && (std::string(av[1]) == "--poke" || std::string(av[1]) == "--activate" ||
                                 std::string(av[1]) == "--destroy" || std::string(av[1]) == "--held")) {
            const std::string o = av[1];
            (o == "--poke" ? poke : o == "--activate" ? activate : o == "--destroy" ? destroy : held) = true;
            av.erase(av.begin() + 1);
        }
        poke = poke || destroy;
        if (av.size() < 3) {
            std::fprintf(stderr, "usage: cocoahost [--poke | --destroy] [--held] [--activate] BUNDLE <minihost arguments>\n");
            return 2;
        }
        NSApplication* app = [NSApplication sharedApplication];
        [app setActivationPolicy:NSApplicationActivationPolicyRegular];
        NSWindow* w = [[NSWindow alloc] initWithContentRect:NSMakeRect(40, 40, 240, 120)
                                                  styleMask:NSWindowStyleMaskTitled
                                                    backing:NSBackingStoreBuffered
                                                      defer:NO];
        [w setTitle:@"cocoahost"];
        if (activate) {
            [w makeKeyAndOrderFront:nil];        // the host in front, as Resolve when its button is pressed
            [app activate];
        } else {
            [w orderFront:nil];
        }
        [NSEvent addLocalMonitorForEventsMatchingMask:NSEventMaskAny
                                              handler:^NSEvent*(NSEvent* e) {
                                                  const NSEventType t = [e type];
                                                  if (t != NSEventTypeAppKitDefined && t != NSEventTypeApplicationDefined &&
                                                      t != NSEventTypePeriodic && t != NSEventTypeSystemDefined)
                                                      say("input %lu %.3f\n", static_cast<unsigned long>(t), now_wall());
                                                  return e;
                                              }];
        [NSTimer scheduledTimerWithTimeInterval:0.1
                                        repeats:YES
                                          block:^(NSTimer*) { say("tick %.3f\n", now_wall()); }];
        __block std::vector<char*> args = av;
        [NSTimer scheduledTimerWithTimeInterval:0.5
                                        repeats:NO
                                          block:^(NSTimer*) {
                                              dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 2 * NSEC_PER_SEC),
                                                             dispatch_get_main_queue(), ^{
                                                                 say("block %.3f\n", now_wall());
                                                                 if (g_poke) g_poke("clip", "main-queue block");
                                                             });
                                              pthread_t th;
                                              if (poke)
                                                  pthread_create(&th, nullptr, [](void*) -> void* {
                                                      usleep(3000000);
                                                      if (g_poke) g_poke(destroy ? "destroy" : "press", "background thread");
                                                      return nullptr;
                                                  }, nullptr);
                                              if (held) {
                                                  // a left button and the key "s" pressed before the wait (the host
                                                  // dispatched them); their releases come during it
                                                  NSEvent* md = [NSEvent mouseEventWithType:NSEventTypeLeftMouseDown
                                                                                   location:NSMakePoint(60, 60)
                                                                              modifierFlags:0
                                                                                  timestamp:[[NSProcessInfo processInfo] systemUptime]
                                                                               windowNumber:[w windowNumber]
                                                                                    context:nil
                                                                                eventNumber:0
                                                                                 clickCount:1
                                                                                   pressure:1.0];
                                                  NSEvent* kd = [NSEvent keyEventWithType:NSEventTypeKeyDown
                                                                                 location:NSZeroPoint
                                                                            modifierFlags:0
                                                                                timestamp:[[NSProcessInfo processInfo] systemUptime]
                                                                             windowNumber:[w windowNumber]
                                                                                  context:nil
                                                                               characters:@"s"
                                                              charactersIgnoringModifiers:@"s"
                                                                                isARepeat:NO
                                                                                  keyCode:1];
                                                  [NSApp sendEvent:md];
                                                  [NSApp sendEvent:kd];
                                              }
                                              say("cocoahost: session start %.3f\n", now_wall());
                                              const int rc = minihost_main(static_cast<int>(args.size()), args.data());
                                              say("cocoahost: session done rc=%d %.3f\n", rc, now_wall());
                                              if (poke) pthread_join(th, nullptr);
                                              // let the queued block / ticks run, then quit
                                              dispatch_after(dispatch_time(DISPATCH_TIME_NOW, NSEC_PER_SEC / 2),
                                                             dispatch_get_main_queue(), ^{
                                                                 [NSApp terminate:nil];
                                                             });
                                          }];
        dispatch_async(dispatch_get_main_queue(), ^{
            say("cocoahost: pid %d ready %.3f\n", static_cast<int>(getpid()), now_wall());
        });
        [app run];
    }
    return 0;
}
