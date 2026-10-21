// The machine's logical cores: how many threads can run at once, so a program
// can size its loading pool (0448 point 2: the pool gets one thread fewer, the
// caller being the last).
//
//     uint32_t threads = voe_platform_cores() - 1;
//
// LOGICAL, AND THE ONES ONLINE NOW. A hyperthread counts as a core, since a
// pool of loading tasks gains from it. Linux answers the cores online, not
// those the process's affinity mask allows; a program pinned to fewer gets a
// pool too big for its share, which costs waiting and not correctness. Asking
// sched_getaffinity instead would lift that. Windows answers every processor
// group's active cores.
//
// AT LEAST 1. A count the OS will not give is 1: a pool of no threads still
// runs every task on the caller, so the answer is always safe to size by.
//
// ASKED, NOT CACHED. Every call asks the OS again, so a core taken offline
// shows in the next answer. It is a system call: ask once per pool made, not
// once per task.
#pragma once

#include <stdint.h>

uint32_t voe_platform_cores(void);
