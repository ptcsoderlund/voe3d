# 06 — The view and the sun's shadows draw the foliage
folder: 3d
after: 05
decisions: 0168, 0388, 0421, 0442

## Change
0442 point 5, the drawing: each gathered group is one instanced draw in the camera pass and
in each cascade of a light no blocker holds.

- `3d/src/draw_foliage.h`, `3d/src/draw_foliage.c` —
  - `bool voe_3d_draw_foliage_solid(voe_render_device *, const voe_3d_frame *)`: every group,
    `rest` ones too, through `voe_render_frame_draw_instanced` with its part's record as
    `draw_models.c` draws a solid part (its worn material's record, colour white), two-sided
    when that material is (`voe_render_frame_two_sided` around it, as `draw_group.c` does),
    all inside one `foliage` span opened at the first draw (0388: the work names its part of
    the frame). Nothing with none. False when render refuses a draw, which stops here.
  - `bool voe_3d_draw_foliage_cast(voe_render_device *, const voe_3d_frame *)`: every group not
    `rest` whose material casts (`voe_3d_draw_casts`), into the open shadow pass.
  - Header points: one draw per group, why the band's `rest` groups do not cast (shadows draw
    the camera's level, 0425 point 5), why foliage is not in the bounce or point shadows.
- `3d/src/draw_system.c` — `_run` draws the foliage right after the landscape rows' nodes,
  solid, in the world layer. Its header gains one line naming it.
- `3d/src/draw_shadows.c` — in each light's cascades (`draw_light_cascades`), after the
  casters, the foliage cast when `frame_light_blockers` for that light is nought; never in the
  point-shadow pass or the bounce. Its header's casters paragraph gains a line.
- `3d/tests/draw_foliage.c` — new cases on card 05's world seen from the camera above with a
  casting sun from above: the pixel over a copy's place differs from the same frame with no
  copies; the frame's `voe_render_frame_draw_count` with three copies equals it with one, and
  exceeds the frame with none.
- `3d/src/src.md`, `3d/tests/tests.md` — entries for `draw_foliage.*`, `draw_system.c` and
  `draw_shadows.c` mention foliage.

## Done when
Tests `3d/draw_foliage`, `3d/shadows`, `3d/terrain_shadow` and `3d/draw_system` pass.
