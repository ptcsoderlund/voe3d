# 01 — The program knows its own path
folder: platform
decisions: 0168, 0247, 0248, 0266

## Change
The game will read its sounds beside its own program (0266 point 3); nothing in `platform`
answers where that is yet. `argv[0]` is not it (a bare name found on PATH, or relative to a
working directory that may have moved).

- `platform/include/platform/path.h` — add
  `[[nodiscard]] const char *voe_platform_path_program(voe_base_arena *arena);`
  the running program's absolute path, UTF-8, pushed into `arena`; NULL, reported, when the
  system will not say. The header's list gains it; the comment says why not `argv[0]`.
- `platform/src/path_wayland.c` — `readlink("/proc/self/exe")`, growing its buffer until the
  answer fits (the link has no length limit of its own worth trusting).
- `platform/src/path_win32.c` — `GetModuleFileNameW(NULL, …)`, growing until it is not
  truncated, made UTF-8 through `wide_win32.h` (0248).
- `platform/tests/path.c` — a new check: the answer is non-NULL, resolving it with
  `voe_platform_path_absolute` gives the same bytes back, `voe_platform_file_exists` is true
  for it, and its last name (`voe_platform_path_name`) starts with `voe_test_platform_path`.
- `platform/platform.md` — the `path.h` entry mentions the program's own path.
- `platform/src/src.md`, `platform/tests/tests.md` — the three entries mention the new call.

## Done when
1. `bash ~/.claude/skills/checks/scripts/checks.sh --folder platform` prints `FINDINGS: 0`
   (runs `platform/path`).
