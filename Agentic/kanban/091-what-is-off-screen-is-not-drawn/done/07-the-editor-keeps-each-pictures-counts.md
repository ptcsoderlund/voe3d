# 07 — The editor keeps each picture's counts
folder: editor
after: 06
decisions: 0168, 0464, 0465

## Change
0465 point 1, the keeping: every picture the editor draws records `_run`'s counts, and the
foliage is gathered once every sun is known (0464 point 4).

- `editor/src/view.h` — `voe_editor_views` gains the pictures drawn this frame, in drawing order:
  `voe_editor_picture_counts pictures[VOE_EDITOR_VIEWS + 1]` and `uint32_t picture_count`, each
  `{ voe_3d_view_counts counts; uint32_t view; bool preview; }` (the view's index, or the preview).
  A comment says only view_passes writes them and the Frame panel reads them.
- `editor/src/view_passes.c` —
  - In the preview and in each shown view: `voe_3d_draw_system_foliage` moves after
    `voe_3d_draw_system_light_blockers`, `voe_3d_draw_system_lights` and
    `voe_3d_draw_system_point_lights`, still before the shadows; those three move to just after
    the `foliage` arena mark, and the `mark` is taken after the gather, so the groups and the
    lights both outlive the pass. The comment that says why the gather comes first says it now
    follows the lights so it knows every sun.
  - The frame handed to `voe_3d_draw_system_run` (in each view, `drawn`, built anew) carries
    `counts` from the gathered and shadowed frame, as it carries `foliage`.
  - `_preview` sets `picture_count` to nought first, then adds the preview's counts when it draws;
    `_draw` adds one per view it draws, with its index. A refused pass adds nothing for it.
- `editor/src/view_passes.h` — the two functions' comments say they record each picture's counts
  in `views`, and that the gather follows the lights.
- `editor/src/src.md` — the `view.h` and `view_passes.*` entries follow.

## Done when
The editor builds, and `grep -n "picture_count" editor/src/view_passes.c` prints a line.
