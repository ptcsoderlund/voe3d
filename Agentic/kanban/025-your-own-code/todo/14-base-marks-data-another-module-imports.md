# 14 — Base marks the engine data a project's library imports
folder: base
decisions: 0168, 0243, 0245

## Change
Point 2 of 0245. Nothing in the tree uses the macro yet; cards 15–17 do.

- `base/include/base/imported.h` (new) — `VOE_BASE_IMPORTED`: expands to
  `__declspec(dllimport)` when both `_WIN32` and `VOE_BASE_IMPORTING` are defined, to nothing
  otherwise. The header comment says: why (on the MSVC ABI a DLL reads another module's data only
  through dllimport; functions bind through the import library without it, 0245); where it goes
  (before the type of every `extern` object in a public header); who defines `VOE_BASE_IMPORTING`
  (`cmake/game.cmake`, for the target `project` on Windows only, never the engine); the constraint
  that such an address is not a constant, so a project takes it at run time. A short example of one
  declaration.
- `base/include/base/base.md` — an entry for `imported.h`.

## Done when
1. `checks.sh --folder base` prints `FINDINGS: 0`.
