# 51 — The machine's cores are counted
folder: platform
after: none
decisions: 0168, 0381, 0448

## Change
0448 point 2: a program sizes its loading pool by the machine's logical cores.

- `platform/include/platform/cores.h` — new: `uint32_t voe_platform_cores(void)`, the logical
  cores this process may run on now, at least 1. Header: what it is for (the loading pool's
  size, 0448), that a count the OS will not give is 1, and that it is asked, not cached.
- `platform/src/cores_wayland.c` — new: `sysconf(_SC_NPROCESSORS_ONLN)`, 1 when not above 0.
  Linux only despite the name, as every `_wayland` file here is.
- `platform/src/cores_win32.c` — new: `GetActiveProcessorCount(ALL_PROCESSOR_GROUPS)`, 1 when 0.
  Written, not built here (0381).
- `platform/include/platform/platform.md`, `platform/platform.md` — entries for `cores.h`.
  `platform/src/src.md` — entries for both files.
- `platform/tests/cores.c` — new: the count is at least 1 and at most 4096, and two asks agree.
- `platform/tests/tests.md` — an entry for `cores.c`.

## Done when
`ctest --test-dir build/debug -R '^platform/cores$'` passes.
