# 03 — The camera lays the forest's depth, then shades it
folder: 3d
after: 02
decisions: 0168, 0508, 0464

## Change
`voe_3d_draw_foliage_solid` draws every seen group twice inside the span it draws it in now
(0508 point 1). Cascades, bounce sun maps and every other drawable are untouched.

- `3d/src/draw_foliage.c`: in `voe_3d_draw_foliage_solid` (and its per-span helper), for each span
  (`foliage`, then `foliage blend`): set `VOE_RENDER_DEPTH_STEP_LAY` and draw the span's groups as
  now, two-sided where each one's worn material is; then `VOE_RENDER_DEPTH_STEP_SHADE` and draw the
  same groups in the same order with the same two-sidedness and records (a blending group's ground
  filled both times); then `VOE_RENDER_DEPTH_STEP_WHOLE` before the span ends. A refused draw still
  stops the walk, the step set back to whole first. Header: a paragraph that the camera lays then
  shades, why (each hidden leaf costs a texture read, each kept pixel is shaded once), and that the
  span holds both steps so its line is the whole cost.
- `3d/src/draw_foliage.h`: `voe_3d_draw_foliage_solid`'s comment says each group is laid then
  shaded; ONE DRAW PER GROUP says two in the camera pass, one in the others.
- `3d/tests/draw_foliage.c`: add `the_camera_lays_then_shades`: a world whose seen groups are G
  draws G × 2 foliage draws in the camera pass and the same triangles as G whole draws would, and
  the cascades' draws are unchanged. Any existing draw-count check of the camera pass follows,
  each with a comment naming 0508; no picture check changes (the dissolve and blend pictures must
  pass as they are).
- `3d/tests/tests.md` (`draw_foliage.c` entry) and `3d/src/src.md` (`draw_foliage.c` entry): the
  camera lays then shades.

## Meaning
- Holds: the LOD dissolve looks exactly as now; shadows never cast a band's `rest` copies; on the
  swap's own frame the incoming copy covers no pixel but casts. Lay and shade use the same groups
  and dither, so this holds.
- Holds: each view's LOD memory sees what it saw: culling and tiers are unchanged (0501).
- Holds: Play and the shipped build draw through this one frame path (0375 point 5).
- Scout: the camera draws only seen groups, in the spans `foliage` and `foliage blend`
  (`draw_foliage.c` around 521–551).

## Done when
- `ctest --test-dir build/debug -R '^3d/(draw_foliage|draw_levels|bounce_foliage)$'` exits 0,
  with `the_camera_lays_then_shades` in `draw_foliage`.
- Human (feature.md How to test 1–5), release build only, lumberhill_project, Frame breakdown:
  1. at the earlier screenshot's stand of about 120 trees, note "view target 1: foliage";
  2. side panels closed, scene view filling the window: foliage 3 ms or less, Total well under
     16.6 ms;
  3. fly into the trees until leaves fill the view: foliage at or under 3 ms, no stutter;
  4. against the earlier look: leaves as full, cut edges as sharp, shadows the same, LODs swap
     calmly flying away;
  5. Play, walk into the forest: the same look, as smooth.
