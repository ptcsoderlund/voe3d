# 01 — Files and folders take any name on Windows
folder: platform
decisions: 0168, 0247, 0248

## Change
The first half of 0248: the conversion pair, then the file and folder calls on it. Windows code is
written and verified on Linux only (ADR-0130). Read the headers of the files below and
`platform/src/window_win32.c`'s title conversion, which is the pattern.

- `platform/src/wide_win32.h` + `wide_win32.c` (new, internal, never included outside `src/`):
  - `[[nodiscard]] bool voe_platform_wide_from_utf8(const char *text, wchar_t *out, int capacity)`:
    false when the text does not fit `capacity` wide units, terminator included.
  - `char *voe_platform_utf8_from_wide(const wchar_t *text, voe_base_arena *arena)`: a
    NUL-terminated UTF-8 copy in the arena.
  - The header: why the "W" calls and not a manifest, flags 0 and U+FFFD (0248), and why a path
    that does not fit the stack buffer is the caller's ordinary failure.
- `platform/src/file_win32.c`: `CreateFileW`, `GetFileAttributesW`, `DeleteFileW`, `MoveFileExW`.
  The path and its `.partial` sibling each go through a `MAX_PATH` wide stack buffer. A path that
  does not fit fails like an open that fails: read UNAVAILABLE, exists false, and write as it
  fails today when the create fails. The header drops the "spelled in ASCII" premise (0247) and
  says a path is UTF-8 converted on the way in.
- `platform/src/folder_win32.c`: `FindFirstFileW`/`FindNextFileW` with `WIN32_FIND_DATAW`, each
  entry's name made UTF-8 into the listing's arena with the same byte-order sort as today;
  `CreateDirectoryW`; `GetEnvironmentVariableW` for `%USERPROFILE%` and `%APPDATA%`, the value made
  UTF-8 into the caller's arena. The header drops the ASCII premise too.
- `platform/tests/file.c`: a new case that writes, checks `voe_platform_file_exists` on, and reads
  back a file named `Min scen ÅÄÖ 李.txt` inside a made folder `Åsa 李 värld` under the test's
  working directory. It uses only `platform` calls and cleans up the way the existing cases in that file do.
- `platform/tests/folder.c`: a new case that makes `Åsa 李 värld` and in it a folder `Östen 张`
  and a file `å.txt`, then lists it. It checks that both names come back byte for byte with
  folder answered right, and cleans up as the existing cases do.
- `platform/src/src.md`: the new pair's entries; `file_win32.c`'s and `folder_win32.c`'s entries
  name the "W" calls and no longer say "why the ASCII call".
- `platform/tests/tests.md`: `file.c` and `folder.c` also check a non-ASCII name.

## Done when
1. `bash ~/.claude/skills/checks/scripts/checks.sh --folder platform` prints `FINDINGS: 0`. It
   runs `platform/file` and `platform/folder` with the new cases.
2. `! grep -nE '\b(CreateFileA|GetFileAttributesA|DeleteFileA|MoveFileExA|FindFirstFileA|FindNextFileA|WIN32_FIND_DATAA|CreateDirectoryA|GetEnvironmentVariableA)\b' platform/src/file_win32.c platform/src/folder_win32.c`
   exits 0.
