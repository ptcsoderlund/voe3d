// The clock: how long something took, in seconds. One function, and it is the
// half of "files and time" that a card has now asked for — see
// platform/window.h, which is where that promise was written down.
//
//     double before = voe_platform_clock_now();
//     ... a frame ...
//     double seconds = voe_platform_clock_now() - before;
//
// IT IS MONOTONIC AND IT IS NOT THE TIME OF DAY. It counts from an origin this
// header deliberately does not name — the machine starting, most likely — so a
// single reading means nothing on its own and only the difference between two of
// them is worth anything. That is the whole point: the wall clock jumps when a
// time server corrects it, when the timezone changes and twice a year, and a
// frame loop that measured itself against one would report a frame that took
// minus an hour. This one never goes backwards and never jumps.
//
// SECONDS, AS A double, BECAUSE THE ENGINE'S UNIT IS THE SECOND. The underlying
// counters are integers of nanoseconds or of ticks, and they are converted here
// rather than at every call site. A double holds a nanosecond of a number of
// seconds well past any run anyone will sit through, so nothing is lost in the
// conversion that the resolution below has not already lost.
//
// THE RESOLUTION IS THE MACHINE'S AND IT IS NOT PROMISED. Nanoseconds on Linux,
// a hundred nanoseconds or so on Windows, and in both cases far finer than a
// frame. It is not fine enough to time one function call, and code that wants to
// should be timing a thousand of them.
//
// NO ONE CALLS A SLEEP AND THIS HEADER OFFERS NONE. Waiting is not the same
// question as measuring, and the wait is on the window, not here — see
// voe_platform_window_wait, which wakes on the window's events as well as time.
//
// THE LAUNCH: voe_platform_clock_launched gives the reading of this same clock
// at which the OS started the process, so `now - at` is the process's age. The
// time before main — loading, a scan of a new program — is otherwise invisible
// to the program's own code. Its resolution is the OS's tick (10 ms on Linux).
// False, with *at untouched, when the OS cannot say. Linux only while Windows
// is paused (decision 0339).
#pragma once

double voe_platform_clock_now(void);
[[nodiscard]] bool voe_platform_clock_launched(double *at);
