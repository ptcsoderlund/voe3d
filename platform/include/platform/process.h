// Starting another program, asking without waiting whether it has ended, and
// ending it together with everything it started.
//
//     voe_platform_process game = {0};
//     const char *argv[] = { "cmake", "--build", "Build/debug", NULL };
//     if (!voe_platform_process_start(argv, scratch, &game))
//             return ...;                          // reported already
//     int code;
//     if (voe_platform_process_poll(&game, &code) == VOE_PLATFORM_PROCESS_ENDED)
//             ...;                                 // game is zeroed again
//     voe_platform_process_end(&game);            // before the caller exits
//
// WHY A CHILD AND NOT A THREAD. The game is a program of its own (ADR-0237): it
// links what the editor may not and owns its window and its loop. Running it,
// or the build that makes it, as a separate process keeps the editor alive and
// answering while it runs, and a crash in it is its exit code, not ours.
//
// WHY THE WHOLE GROUP ENDS. `cmake --build` starts Ninja, which starts the
// compiler; ending only the process that was started would leave the rest
// running with no one waiting for them. So the child is put in a group of its
// own — a process group on Linux, a job object on Windows — and ending it ends
// the group.
//
// STDOUT AND STDERR ARE SHARED, STDIN IS NOT READ. The child writes to the
// caller's own stdout and stderr, so what a build prints lands where the
// editor's lines do. Nothing is written to its stdin and nothing it prints is
// captured.
//
// BEFORE ITS OWN EXIT A CALLER ENDS WHAT IT STARTED. A process still running
// when the caller exits is left behind on Linux; on Windows the job's
// kill-on-close ends it, but nothing waits for it. Call _end on every struct
// that is not zeroed.
//
// Constraints: one struct names one running program; it is the caller's and
// is copied by value only while zeroed. Not thread-safe per struct.
#pragma once

#include <base/arena.h>

#include <stdint.h>

// The running program's handle: on Linux `handle` is the pid and `job` is
// unused; on Windows they are the process and job HANDLEs. Zeroed is "none".
typedef struct {
	uint64_t handle;
	uint64_t job;
} voe_platform_process;

typedef enum {
	VOE_PLATFORM_PROCESS_RUNNING,
	VOE_PLATFORM_PROCESS_ENDED,
} voe_platform_process_state;

// argv is NULL-terminated; argv[0] is looked up on PATH or taken as a path.
// scratch holds the Windows command line and is rewound before the return.
// out must be zeroed. False, reported naming argv[0], when it cannot start.
[[nodiscard]] bool voe_platform_process_start(const char *const *argv,
					      voe_base_arena *scratch,
					      voe_platform_process *out);

// Never blocks. On ENDED writes the exit code (a death by signal is a non-zero
// code), releases the handle and zeroes the struct. A zeroed struct asserts.
voe_platform_process_state voe_platform_process_poll(voe_platform_process *process,
						     int *exit_code);

// Ends the program and everything it started, waits for it, and zeroes the
// struct. A zeroed struct is a no-op.
void voe_platform_process_end(voe_platform_process *process);
