# 03 — A program reads its own arguments as UTF-8
folder: platform
decisions: 0168, 0247, 0248

## Change
On Windows `main`'s `argv` is in the legacy code page, so a project folder given on the command
line would arrive broken (0247). This adds 0248's last point. Its one caller, `editor/src/main.c`,
is card 04's; do not touch it.

- `platform/include/platform/arguments.h` (new): a struct `voe_platform_arguments` holding
  `int count` and `const char *const *values`, and
  `voe_platform_arguments voe_platform_arguments_read(int argc, char *argv[], voe_base_arena *arena)`.
  The header says:
  - the values are UTF-8 on both platforms, and `values[count]` is NULL;
  - Linux hands back `argv` itself, so the arena is not used there;
  - Windows ignores `argv` and reads the process's wide command line;
  - the rule 6 deviation for `char *argv[]`: it is `main`'s own signature.
- `platform/src/arguments_wayland.c`: `argv` as it is.
- `platform/src/arguments_win32.c`: `GetCommandLineW`, `CommandLineToArgvW`, then each entry
  made UTF-8 into the arena with `voe_platform_utf8_from_wide` (`src/wide_win32.h`), and
  `LocalFree`. If `CommandLineToArgvW` fails, fall back to `argv`.
- `cmake/voe.cmake`: the `WIN32` branch of the platform backend links `shell32` beside `user32`.
  Its comment mentions the Win32 libraries; keep it true.
- `platform/tests/arguments.c` (new): a hand-made argv of three strings, one of them `Åsa 李`,
  comes back with count 3, the same bytes and a NULL after the last. Linux only by construction;
  the header says so.
- `platform/platform.md`, `platform/include/platform/platform.md`, `platform/src/src.md` and
  `platform/tests/tests.md`: an entry each for the new files. `platform.md`'s opening list of what
  the OS gives now includes the program's own arguments.

## Done when
1. `bash ~/.claude/skills/checks/scripts/checks.sh --folder platform` prints `FINDINGS: 0`. It
   builds and runs `platform/arguments`.
2. `grep -n 'shell32' cmake/voe.cmake` prints the platform backend's `WIN32` link line.
