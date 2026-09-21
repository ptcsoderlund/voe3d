# 09 — The gizmo header stands on 3d's own dependencies
folder: 3d
decisions: 0168, 0205, 0208

## Change
Clears the findings that are 015's own (ADR-0208). Comments, one include and index entries only: no
signature, no type and no code line changes.

- `3d/include/3d/gizmo.h` — drop `#include <platform/window.h>`; `voe_platform_size` already arrives through
  `render/device.h`, as it does in `pick.h` and `outline.h`. The header comment is 83 lines; bring it to 60
  or fewer by moving each paragraph that explains one function (the why of `voe_3d_gizmo_at`, `_hit`,
  `_grab`, `_quads` and the like) down to the comment above that function's declaration. The top keeps what
  the file does, the usage example and the constraints that hold for the whole file. Say nothing new.
- `3d/tests/gizmo.c` — drop `#include <platform/window.h>`; nothing else.
- `3d/include/3d/3d.md` — add an entry for `gizmo.h`, one sentence under 300 characters, in the same voice
  as its neighbours: the move gizmo's size, hit test, grab point and triangles as arithmetic.
- `3d/src/src.md` — the `gizmo.c` entry is 324 characters; shorten it under 300. Whatever it loses that
  the file's header does not already say goes into `3d/src/gizmo.c`'s header comment (28 lines now; keep
  it under 60).

Read those four files and the header of `3d/src/gizmo.c`; open no other.

## Done when
- `cmake -P check.cmake` exits 0 (its `includes` step no longer names `3d/include/3d/gizmo.h` or
  `3d/tests/gizmo.c`).
- `bash ~/.claude/skills/checks/scripts/checks.sh --structure | grep -E 'FINDING 3d/(include/3d/(3d\.md|gizmo\.h)|src/(src\.md: entry .gizmo|gizmo\.c)|tests/gizmo\.c)'`
  prints nothing.
- `checks.sh --folder 3d` exits 0.
- `git diff -U0 -- 3d/include/3d/gizmo.h 3d/tests/gizmo.c 3d/src/gizmo.c` shows no added line that is
  not a comment or blank.
