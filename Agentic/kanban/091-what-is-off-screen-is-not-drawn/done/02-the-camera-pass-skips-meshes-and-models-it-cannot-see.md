# 02 — The camera pass skips meshes and models it cannot see, and counts them
folder: 3d
after: 01
decisions: 0168, 0395, 0464

## Change
0464 points 2 and 5 for mesh rows and model rows in `voe_3d_draw_system_run`, and the counts
every later card adds to.

- `3d/include/3d/draw_system.h` —
  - `voe_3d_view_counts` (new): `uint32_t drawn, skipped` (the camera pass's things) and
    `uint32_t cast, cast_skipped` (the cascades', summed over every cascade of every light); what a
    thing is (0464 point 5).
  - `voe_3d_frame` gains `voe_3d_view_counts counts`: _frame zeroes it; the foliage gather and the
    shadows call add their part (later cards); the caller hands it on to `_run` as it does
    `foliage`.
  - `voe_3d_draw_system_run` returns `voe_3d_view_counts`: `frame.counts` plus its own camera
    things. A caller ignoring it is unchanged, so no other folder needs an edit.
  - The "ONE DRAW PER MESH, FROM THE CPU. … no culling" paragraph and _run's comment say a thing
    whose world sphere the view cannot see is skipped and counted, and what is never culled
    (panels, marks, outline, gizmo).
- `3d/src/draw_system.c` — `_run` builds one `voe_3d_cull_of(frame.view, false)` (none for a blind
  frame); the mesh walk takes each mesh row's sphere from `voe_render_geometry_box` (centre and
  half diagonal) under the row's object matrix about the eye at the lag, as the draw uses it, and
  skips the row when `voe_3d_cull_sees` says no: not drawn, not held, `skipped` + 1; a mesh row
  that draws or is held is `drawn` + 1. Panels are walked as now. File header gains a line.
- `3d/src/draw_models.h`, `3d/src/draw_models.c` — `voe_3d_draw_models` gains
  `const struct voe_3d_cull *cull` (NULL tests nothing) and `voe_3d_view_counts *counts`: a model
  row (not a landscape) whose entry's `centre`/`radius` under its world matrix about the eye is not
  seen draws and holds none of its parts; each model row is one thing, drawn or skipped.
- `3d/tests/view_culling.c` (new), on a headless device as `3d/tests/draw_system.c` builds one, a
  camera at the origin looking down −Z:
  - three cubes, 5 m ahead, 5 m behind and 50 m to the left: `_run` returns drawn 1, skipped 2,
    and the centre pixel is the front cube's colour;
  - a cube straddling the right edge of the picture is drawn and its pixel inside the edge shows;
  - the same with a model row (a `.glb` loaded as `3d/tests/draw_levels.c` loads one) behind the
    camera: skipped 1.
- `3d/src/src.md`, `3d/tests/tests.md`, `3d/3d.md` (`draw_system.h` entry) — entries follow.

## Done when
Tests `3d/view_culling`, `3d/draw_system` and `3d/draw_levels` pass.
