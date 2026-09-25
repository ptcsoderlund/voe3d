# 02 — Paths, libraries and a program's output file take any name on Windows
folder: platform
decisions: 0168, 0247, 0248

## Change
The rest of 0248's calls, on card 01's `platform/src/wide_win32.h` (read its header). Windows code
is written and verified on Linux only (ADR-0130).

- `platform/src/path_win32.c`: `GetFullPathNameW` then `GetFileAttributesW`. The input goes
  through a `MAX_PATH` wide stack buffer (too long: NULL, as for a name that is not there); the
  resolved wide path is made UTF-8 into the caller's arena. The header drops the "spelled in
  ASCII" premise.
- `platform/src/library_win32.c`: `LoadLibraryW` on the name through a `MAX_PATH` wide buffer
  (too long: NULL with the same report). `FormatMessageW`'s text is made UTF-8 before it goes into
  the report, so a localised loader message arrives in UTF-8 and not the code page. The header
  loses the "an ASCII file name" reasoning.
- `platform/src/process_win32.c`: the output file is opened with `CreateFileW` through the pair.
  The command-line conversion may move onto the pair. Its asserts on the conversion stay only if
  they still mark a caller's bug; with flags 0, bad UTF-8 is U+FFFD and not a failure.
- `platform/tests/path.c`: a new case makes a folder `Åsa 李 värld` under the working directory.
  `voe_platform_path_absolute` on it is non-NULL, and its `voe_platform_path_name` is those bytes.
  The case cleans up as the existing cases do.
- `platform/src/src.md`: the `path_win32.c`, `library_win32.c` and `process_win32.c` entries name
  the "W" calls.
- `platform/tests/tests.md`: `path.c` also resolves a non-ASCII folder.

## Done when
1. `bash ~/.claude/skills/checks/scripts/checks.sh --folder platform` prints `FINDINGS: 0`. It
   runs `platform/path` with the new case.
2. `! grep -nE '\b[A-Z][A-Za-z]+A\s*\(|WIN32_FIND_DATAA' platform/src/*_win32.c` exits 0: no
   "A" call is left in `platform`.
