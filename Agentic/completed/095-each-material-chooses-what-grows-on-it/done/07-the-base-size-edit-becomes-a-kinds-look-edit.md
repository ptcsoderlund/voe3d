# 07 — The base-size edit becomes a kind's look edit
folder: editor
after: 06
decisions: 0168, 0469, 0476

## Change
0476 points 1 and 7's plumbing, and the editor building again against cards 02 and 05: the
brush keeps Density alone of its sliders, and a kind's base-size edit becomes an edit of its
whole look (`voe_assets_landscape_look`, assets/landscape.h), set live and one undo step when let
go. Card 08 adds the look's other controls.

- `editor/src/models.h`, `editor/src/models.c` — `voe_editor_models_kind_size_set` becomes
  `voe_editor_models_look_set(models, path, kind, voe_assets_landscape_look look)`, forwarding
  to `voe_3d_models_landscape_look_set` (3d/models_foliage.h). Its comment and the header's
  mentions of base size say look.
- `editor/src/strokes.h`, `editor/src/strokes.c` — the base-size stroke becomes a look stroke:
  `kind_size`, `size_before`, `size_after` become `look`, `look_before`, `look_after`;
  `voe_editor_stroke_new_kind_size` becomes `voe_editor_stroke_new_look(path, kind, before,
  after)`; apply sets the look either way. Header and sketch follow.
- `editor/src/sculpt.h`, `editor/src/sculpt.c` —
  - The brush loses `size_vary`, `turn_vary`, `lean` and their range and default constants;
    VOE_EDITOR_SCULPT_FOLIAGE_SLIDERS becomes 1 (Density); the scatter's
    `voe_3d_foliage_brush` is filled without them.
  - The base-size ask and edit become a look ask and edit: `size_asked`, `size_kind`,
    `size_to`, `size_held` become `look_asked`, `look_kind`, `look_to` (a look), `look_held`;
    `sizing*` become `looking*`, `looking_before` a look. Settled as before: set live while held,
    one look stroke when let go if the look changed.
  - The header's paragraph on the base-size edit says a kind's look edit.
- `editor/src/inspector_foliage.h`, `editor/src/inspector_foliage.c` — the Size ±%, Turn ±° and
  Tilt with slope % sliders and their figures go; Density stays. A kind's Base size slider asks
  a look: that kind's current look with only its size changed. Header follows.
- `editor/src/src.md` — the `models.*`, `strokes.*`, `sculpt.*` and `inspector_foliage.*`
  entries follow.

## Done when
The editor builds, and `grep -rnE "size_vary|turn_vary|kind_size_set|new_kind_size"
editor/src/sculpt.h editor/src/sculpt.c editor/src/strokes.h editor/src/models.h` prints
nothing.
