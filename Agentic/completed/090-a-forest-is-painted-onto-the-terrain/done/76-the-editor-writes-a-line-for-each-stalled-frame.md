# 76 — The editor writes a line for each stalled frame
folder: editor
after: 75
decisions: 0168, 0358, 0458

## Change
Bug 08, the measurement in the editor (0458 point 3): every frame that
stalls, on the CPU or on the card, puts one line on stderr naming what took
the time, so the bounce's spike in lumberhill is named before it is fixed.
Read `app/include/app/hitches.h` for the calls.

- `editor/src/frame_hitches.h`, `editor/src/frame_hitches.c` — new: the
  editor's hitch watch.
  - `voe_editor_frame_hitches` — a `voe_app_hitches`, the last clock reading
    (`voe_platform_clock_now`), up to 8 `voe_app_stretch` marks this frame.
    Zeroed is ready.
  - `void voe_editor_frame_hitches_begin(voe_editor_frame_hitches *watch)` —
    at the top of a frame: drops last frame's marks, reads the clock.
  - `void voe_editor_frame_hitches_mark(voe_editor_frame_hitches *watch,
    const char *name)` — ends the stretch since the last reading, named
    `name` (a string that outlives the frame), and reads the clock again.
  - `void voe_editor_frame_hitches_end(voe_editor_frame_hitches *watch,
    const voe_render_device *gpu, double elapsed, double shadows)` — adds
    the stretch `shadows` (within `views`), feeds `voe_app_hitches_frame`
    with the frame's `elapsed`; then, when `voe_render_frame_gpu_time`
    gives a total, feeds `voe_app_hitches_card` with it and
    `voe_render_frame_pass_times` (up to 64 passes); each true writes its
    line to stderr after `editor: hitch: `.
  - The header's points: why (bug 08, 0458; the Frame panel misses a single
    frame); the stretches and what each holds; the card line lags by
    VOE_RENDER_FRAMES_IN_FLIGHT frames, as the Frame panel's does, so a
    frame line and its card line may be two frames apart; always on.
- `editor/src/view_passes.h`, `editor/src/view_passes.c` —
  `voe_editor_view_passes_preview` and `voe_editor_view_passes_draw` gain
  `double *shadows`, last: each adds the seconds its
  `voe_3d_draw_system_shadows` calls took, read with the platform clock
  around each call. Their comments say so.
- `editor/src/main.c` — one `voe_editor_frame_hitches` beside the breakdown;
  `_begin` at the top of the loop body; `_mark` with `open` after the frame
  is opened, `step` after the commands, pointer and world step before the
  draw, `views` after the preview and view passes, `interface` after the
  interface's draw, `end` after the frame is ended and presented; a local
  `double shadows = 0` handed to both view-pass calls; `_end` beside
  `voe_editor_frame_breakdown_take` with `opened.tick.elapsed`. The header's
  last paragraph names the hitch lines. Keep the file under 800 lines.
- `editor/src/src.md` — an entry for the new pair; `view_passes.*` and
  `main.c` follow.

## Done when
The editor builds, and `grep -c "voe_editor_frame_hitches_" editor/src/main.c`
prints at least 7.

The human, in the editor (bug 08's How to reproduce):
1. Start the editor from a terminal, open lumberhill, set the sun's bounces
   to 1. Fly and walk about the hill and paint some foliage. At each felt
   stall, `editor: hitch:` lines appear on stderr.
2. Bounces 0, the same moves: few or no lines.
3. File the lines of step 1 as a new bug of 090 (0458 point 4): the
   stalled stretch or pass names the cause, and its fix is planned from it.
