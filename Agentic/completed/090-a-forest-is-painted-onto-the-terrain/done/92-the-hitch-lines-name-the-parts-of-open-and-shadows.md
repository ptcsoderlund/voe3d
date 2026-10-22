# 92 — The hitch lines name the parts of open and shadows
folder: editor
after: 90, 91
decisions: 0168, 0215, 0458, 0461

## Change
Bug 11, the measurement in the editor (0461 points 1, 3, 4). Read
`app/include/app/hitches.h` for the line's calls.

- `editor/src/frame_hitches.h`, `editor/src/frame_hitches.c`
  - `VOE_EDITOR_FRAME_HITCHES_STRETCHES` 8 → 16.
  - `voe_editor_frame_hitches` gains this frame's `double shadows`,
    `double gather` and a `voe_3d_shadow_times parts`, zeroed by `_begin`;
    the view passes add to them.
  - `_end` loses its `double shadows` parameter: it adds `shadows`, then the
    five parts as `shadows: casters`, `shadows: ground`, `shadows: foliage`,
    `shadows: lamps`, `shadows: bounce`, and `gather`, as stretches, a part
    at 0 left out.
  - The card's line, when written, ends with the newest recorded frame's
    `voe_render_frame_triangle_count` and `voe_render_frame_draw_count`, as
    `, <n> triangles, <m> draws`.
  - The header: the stretches 0461 names and what each holds (`pace` is
    0215's unfocused wait, nought when focused; `fence` the slot's fence);
    the parts within `shadows` and `gather` within `views`; the counts on
    the card line are of the newest frame, not the measured one.
- `editor/src/view_passes.h`, `editor/src/view_passes.c` —
  `voe_editor_view_passes_preview` and `voe_editor_view_passes_draw` take
  `voe_editor_frame_hitches *watch` in place of `double *shadows`. Each
  frame they build sets `clock` to `voe_platform_clock_now`; after its
  `voe_3d_draw_system_shadows` call its `shadow_times` are added to
  `watch->parts` and the call's seconds to `watch->shadows`; the seconds of
  each `voe_3d_draw_system_foliage` call go to `watch->gather`. Comments
  follow.
- `editor/src/main.c` — marks in code order: `open` just before the world
  step (theme, refresh, saves, history), `world` after
  `voe_editor_world_step`, `undo` after `voe_editor_undo_settle`, `pace`
  after `voe_app_frame_open` (the old `open` mark); `fence` after the
  draw's open, before `voe_editor_models_frame`. The `shadows` local and its
  comment go; both view-pass calls get `&hitches`; `_end` drops its last
  argument. The file is at 797 lines: keep it at 800 or under.
- `editor/src/src.md` — the `frame_hitches.*` and `view_passes.*` entries
  follow.

## Done when
The editor builds, and `grep -cE 'hitches_mark\(&hitches, "(world|undo|pace|fence)"' editor/src/main.c`
prints 4.

The human, in the editor (bug 11's How to reproduce):
1. Start the editor from a terminal, open lumberhill's `Hill.landscape`
   scene, fly in among the painted trees and keep the editor focused while
   moving between them. `editor: hitch:` lines appear at each stall.
2. Click the terminal for a few seconds: frame lines with `pace` near
   235 ms appear, and none while the editor is focused.
3. File the lines of step 1 as a new bug of 090 (0461 point 5): the
   stretch, shadow part or card pass named, with the triangle count, is
   what its fix is planned from.
