# 09 — The view passes leave main.c
folder: editor
decisions: 0168, 0203, 0205

## Change
`editor/src/main.c` is 830 lines and cards 12 and 13 change what it draws per view. This card only moves code;
the program draws exactly what it drew.

- `editor/src/view_passes.h` / `view_passes.c` (new) — what a frame draws into the views, out of main.c:
  - the capacity block `EDITOR_CAPACITIES` and the two constants it and the passes use
    (`VOE_EDITOR_OUTLINE_MILLIMETRES`, `VOE_EDITOR_GIZMO_MILLIMETRES`), with their comments, as
    `VOE_EDITOR_CAPACITIES` in the header, so the next capacity a pass needs is added beside the pass;
  - the loop in main.c that opens a pass per shown view, runs `voe_3d_draw_system_run` with its outline and
    gizmo, and ends it (today's "a pass per view the tree shows" block), as one function
    `[[nodiscard]] bool voe_editor_view_passes_draw(...)` that takes by value or pointer what that block reads
    today and returns what `drawn` becomes. Its header: what one call draws, that a refused pass stops the
    rest and returns false, and what the device's capacities must cover (the capacity comment moves here).
- `editor/src/main.c` — calls it where the block was; the file header's "a frame is a pass per view" paragraph
  names view_passes.h for the per-view part. Includes that main.c no longer needs are dropped.
- `editor/src/src.md` — entries for `view_passes.h` and `view_passes.c`; the `main.c` entry if it names the
  passes.

## Done when
The Checks line of `CLAUDE.md` with `{folder}` = `editor` exits 0, `wc -l < editor/src/main.c` prints a
number under 780, and `grep -c "voe_3d_draw_system_run" editor/src/main.c` prints 0.
