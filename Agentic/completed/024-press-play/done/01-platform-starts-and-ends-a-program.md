# 01 — platform starts, polls and ends a program
folder: platform
decisions: 0168, 0237

## Change
A new public header, used by `editor` in card 07. Nothing calls it yet.

- `platform/include/platform/process.h` — new.
  - `voe_platform_process`: a plain struct holding the running program's handle (a pid on Linux, a
    process and a job handle on Windows) as integers wide enough for either; zeroed is "none".
  - `typedef enum { VOE_PLATFORM_PROCESS_RUNNING, VOE_PLATFORM_PROCESS_ENDED } voe_platform_process_state;`
  - `[[nodiscard]] bool voe_platform_process_start(const char *const *argv, voe_base_arena *scratch,
    voe_platform_process *out);` `scratch` holds the Windows command line and is rewound before the
    return. argv NULL-terminated, `argv[0]` found on PATH or given as a path; the child inherits stdout and
    stderr. False, reported through base/report.h naming `argv[0]`, when it cannot be started.
  - `voe_platform_process_state voe_platform_process_poll(voe_platform_process *process, int *exit_code);`
    never blocks; on ENDED writes the exit code (a signal death is a non-zero code), releases the
    handle and leaves the struct zeroed. Polling a zeroed struct asserts.
  - `void voe_platform_process_end(voe_platform_process *process);` ends the program and everything
    it started, waits for it, leaves the struct zeroed; a zeroed struct is a no-op.
  - Header points: why a child and not a thread (the game is a program of its own, 0237); why the
    whole group ends (`cmake --build` starts Ninja, which starts the compiler); stdout and stderr are
    shared, stdin is not read; what a caller must do before its own exit (end what it started).
- `platform/src/process_wayland.c` — new: `posix_spawnp` with a spawn attribute putting the child in
  a new process group; `waitpid(..., WNOHANG)`; end is `SIGTERM` to the group then a blocking
  `waitpid`.
- `platform/src/process_win32.c` — new: the argument list quoted into one command line by the
  `CommandLineToArgvW` rules, `CreateProcessW` (UTF-8 converted as the other `_win32.c` files do)
  inside a job object with kill-on-close; `WaitForSingleObject(…, 0)` and `GetExitCodeProcess`; end
  is `TerminateJobObject`, a wait, and both handles closed.
- `platform/tests/process.c` — new, one test each:
  - `cmake -E true` polled until ENDED gives 0; `cmake -E false` gives non-zero.
  - `cmake -E sleep 30` is RUNNING on the first poll, and `voe_platform_process_end` returns within
    five seconds (timed with `platform/clock.h`) with the struct zeroed.
  - a program name that does not exist is a false start.
- `platform/include/platform/platform.md`, `platform/src/src.md`, `platform/tests/tests.md` — an entry
  for each new file.

## Done when
The Checks line of `CLAUDE.md` with `{folder}` = `platform` exits 0, and `ctest --test-dir
build/debug -R '^platform/process$'` lists and passes the test.
