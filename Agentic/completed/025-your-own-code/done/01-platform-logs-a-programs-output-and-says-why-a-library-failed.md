# 01 — Platform writes a program's output to a file and says why a library would not open
folder: platform
decisions: 0168, 0242

## Change
Point 8 of 0242 needs every build step's output in `Build/build.log`, and point 8's failed load needs
the loader's reason.

- `platform/include/platform/process.h` — `voe_platform_process_start` gains `const char *output`
  after `argv`: NULL keeps today's shared stdout and stderr; a path sends both to that file, opened
  for append and created when missing, stdin still unread. The header's STDOUT AND STDERR paragraph
  and the example say so. The one caller outside this folder, `editor/src/play.c`, is card 05's;
  do not touch it.
- `platform/src/process_wayland.c` — open the file before the fork and `dup2` it onto 1 and 2 in the
  child; an open that fails is refused and reported naming the path, starting nothing.
- `platform/src/process_win32.c` — the same through an inheritable handle in `STARTUPINFO`
  (written, verified on Linux only, ADR-0130).
- `platform/include/platform/library.h` — `voe_platform_library_new` now reports a failure through
  `VOE_BASE_ERROR("platform", ...)`, naming the library and the loader's own message; still NULL.
  The FAILURE paragraph says a caller may show the report. `render/src/loader.c` needs no change.
- `platform/src/library_wayland.c` (`dlerror()`), `platform/src/library_win32.c`
  (`GetLastError` through `FormatMessage`) — that report.
- `platform/tests/process.c` — the existing calls pass NULL; a new case starts `sh -c` printing one
  line to stdout and one to stderr with an output path in a `mktemp`-style folder under the test's
  working directory, polls to ENDED, and reads the file back holding both lines; a second start
  with the same path appends.
- `platform/tests/library.c` (new) — `voe_platform_library_new` on a name no system has returns
  NULL and `voe_base_report_error_first()` names it.
- `platform/tests/tests.md` (the new `library.c`), `platform/include/platform/platform.md` and
  `platform/src/src.md` — the entries for process and library that change.

## Done when
1. `checks.sh --folder platform` prints `FINDINGS: 0` (it builds and runs the tests `platform/process`
   and `platform/library`).
