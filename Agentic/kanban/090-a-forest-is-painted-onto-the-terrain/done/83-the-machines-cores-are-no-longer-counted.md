# 83 — The machine's cores are no longer counted
folder: platform
after: 79, 82
decisions: 0168, 0381, 0457, 0459

## Change
Bug 09, 0459 point 4: the core count sized only the loading pool; cards 79 and 82 took away its
two callers.

- `platform/include/platform/cores.h`, `platform/src/cores_wayland.c`,
  `platform/src/cores_win32.c`, `platform/tests/cores.c` — deleted (`git rm`).
- `platform/platform.md` — the `include/platform/cores.h` entry goes.
- `platform/include/platform/platform.md` — the `cores.h` entry goes.
- `platform/src/src.md` — the `cores_wayland.c` and `cores_win32.c` entries go.
- `platform/tests/tests.md` — the `cores.c` entry goes.

## Done when
`test ! -e platform/include/platform/cores.h && ! grep -rnE "cores\.h|voe_platform_cores|cores_(wayland|win32)"
platform game editor` exits 0.
