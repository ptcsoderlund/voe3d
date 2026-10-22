# 06 — Foliage copies are gathered seen, cast or unseen
folder: 3d
after: 05
decisions: 0168, 0442, 0450, 0464

## Change
0464 point 4: one gather a view sorts each copy into a tier; the camera draws the seen, the
cascades the seen and cast, the bounce sun maps all three, so the bounce is unchanged.

- `3d/include/3d/draw_system.h` —
  - `voe_3d_foliage_tier` (new enum): seen, cast, unseen; `voe_3d_foliage_group` gains `tier`.
  - `VOE_3D_FOLIAGE_GROUPS` becomes 768 (three tiers of 256); its comment says so.
  - `voe_3d_draw_system_foliage`'s comment: it runs after `voe_3d_draw_system_lights`, so it
    knows every sun; a copy's sphere is its kind's entry `centre`/`radius` under the copy's world
    matrix about the eye; seen when the view's cull sees it, else cast when a cascade of a casting
    sun of the frame (`frame->light` and each of `more_lights` whose light row casts) sees it with
    its sun side open, else unseen; the frame's `counts` gain each copy not `rest`, seen as
    `drawn`, the others `skipped`.
- `3d/src/draw_foliage.h`, `3d/src/draw_foliage.c` —
  - `struct voe_3d_foliage_key` gains the tier; equal keys are one group as before.
  - The gather builds `voe_3d_cull_of(frame->view, false)` and, per casting sun, the four
    `voe_3d_cull_of(cascades.light[c], true)` from `voe_3d_shadow_cascades_fit(frame->view,
    frame->eye, direction, VOE_3D_SHADOW_TEXELS)` with the direction `draw_shadows.c`'s
    `draw_light_cascades` fits with; level choice per copy is unchanged.
  - `voe_3d_draw_foliage_solid` draws seen groups only.
  - `voe_3d_draw_foliage_cast` gains `bool every_tier` and `voe_3d_view_counts *counts` (NULL
    counts nothing): seen and cast groups, or all three with `every_tier`; with counts, the copies
    drawn added to `cast` and the unseen ones to `cast_skipped`, `rest` groups not counted.
  - Header: the tiers, why a copy is sorted once a view and not per pass (an instanced group
    cannot skip copies), why the bounce draws every tier.
- `3d/src/draw_shadows.c` — the cascades' foliage cast: `every_tier` false, the frame's counts.
- `3d/src/draw_bounce.c` — the sun maps' foliage cast: `every_tier` true, NULL.
- `3d/tests/draw_foliage.c` — cases on the existing world:
  - every copy behind the camera: the gather counts them all skipped, none drawn;
  - copies behind the camera with a low sun beyond them: the ground pixel in view where their
    shadow falls is darker than the same frame with no copies;
  - copies ahead: counted drawn, and the pixel over a copy still differs from the frame with no
    copies.
- `3d/src/src.md`, `3d/tests/tests.md` — entries follow.

## Done when
Tests `3d/draw_foliage`, `3d/bounce_foliage`, `3d/shadow_culling` and `3d/view_culling` pass.
