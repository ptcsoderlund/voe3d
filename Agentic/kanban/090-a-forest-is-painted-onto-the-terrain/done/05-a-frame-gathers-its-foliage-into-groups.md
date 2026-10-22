# 05 — A frame gathers its foliage into groups
folder: 3d
after: 04
decisions: 0168, 0375, 0425, 0432, 0442

## Change
0442 point 5, the gathering: once a frame per view, every copy given its level and written as
a render instance into its group. Nothing draws the groups yet (card 06).

- `3d/include/3d/draw_system.h` (public, at its header cap: comments go above what they
  describe) —
  - The header's "ONE DRAW PER MESH … No instancing" paragraph says foliage is the exception,
    reworded within the cap.
  - `VOE_3D_FOLIAGE_INSTANCES` (1u << 17): instances a frame, every view together, a
    program's `instances` capacity. `VOE_3D_FOLIAGE_GROUPS` (256): groups one frame's view
    gathers at most, which a program adds to its objects for each pass drawing foliage.
  - `voe_3d_foliage_group`: `voe_render_geometry geometry`, `const voe_3d_model_part *part`
    (its record and sides), `uint32_t first, count` (render instances), `bool rest` (a
    band's level-before copies).
  - `voe_3d_frame` gains `const voe_3d_foliage_group *foliage` and `foliage_count`; `_frame`
    leaves them NULL and 0.
  - `[[nodiscard]] bool voe_3d_draw_system_foliage(const voe_ecs_world *, voe_render_device *,
    voe_3d_frame *, voe_base_arena *)`: inside an open frame, before any pass; the groups on
    `arena`, which must outlive the frame's passes. For each model row wearing a loaded
    landscape that `_run` draws solid (the same rows and world place `draw_system.c`'s
    landscape walk takes, off, hidden and fading rows left out), each copy whose kind is a
    loaded `.glb` in `frame->models` (each kind found once a row): its world transform the
    row's composed with its placed one (`voe_scene_transform_compose`), its level and share by
    `voe_3d_model_level_choose` with the frame's view and eye; one instance per opaque or
    cut-out part the level has, the top three rows of `voe_scene_transform_matrix` about the
    eye, dithered as `draw_models.c` dithers a dissolving row; under a whole share also one in
    the level before with the other dither, in a `rest` group. Instances are counted per
    group, then filled contiguously and written with one `voe_render_frame_instances`. False
    with none when render refuses the room; groups past VOE_3D_FOLIAGE_GROUPS are left out;
    each said once on stderr. True with none with no store or no landscape row.
- `3d/src/draw_foliage.h`, `3d/src/draw_foliage.c` — new, internal, each with its header: the
  call above (draw_system.c stays under ~700 lines), its group key (kind, level, part, rest).
- `3d/tests/draw_foliage.c` — new, headless, a device with `instances` 64: a store holding the
  `.glb` of `3d/tests/model_data.inc` (as `3d/tests/draw_model_fade.c` loads it) and an
  8-cell flat landscape naming it as kind 0 with three copies, a world with a camera 5 m above
  and a model row wearing the landscape. Inside a frame: the call gives groups whose counts sum
  to three times the model's opaque or cut-out parts, none `rest`, each group's first and
  count within what was written; the row switched off gives none; a store without the `.glb`
  gives none; with `instances` 2 the call is false with none.
- `3d/3d.md`, `3d/src/src.md`, `3d/tests/tests.md` — entries; `draw_system.h`'s entry names
  the foliage phase.

## Done when
Tests `3d/draw_foliage`, `3d/draw_system` and `3d/draw_levels` pass.
