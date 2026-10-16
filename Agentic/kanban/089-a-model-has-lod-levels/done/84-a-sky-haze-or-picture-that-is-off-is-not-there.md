# 84 — A Sky, Haze or Picture that is off is not there
folder: 3d
after: 83
decisions: 0168, 0427, 0428, 0440

## Change
Bug 10: the haze, sky and exposure ignore the Enabled switch. Every
caller (`3d`'s own `voe_3d_draw_system_frame`, the editor's
`editor/src/view_passes.c`, the game's frame, Play and Ship) takes these
rows through two functions, so the fix is there, once (0440 point 2).

`3d/src/draw_system.c` (650 lines; open only its header comment, lines
1-3, and lines 95-165):
- `voe_3d_draw_system_sky` takes, from the Haze table and from the Sky
  table separately, the first row whose entity
  (`voe_scene_haze_entities` / `voe_scene_sky_entities`) is on by
  `voe_scene_identity_off` (`scene/identity_component.h`), in place of
  `rows[0]`; with none on, that part is zeroed as with no row.
- `voe_3d_draw_system_picture`: the same over `voe_scene_picture_entities`;
  none on is the zeroed x1 and Clip.
- One small static shared by the three, if it reads clearer: the index of
  the first on entity in a list, or the count when none. Its comment
  names 0440.
- The file's header comment (lines 1-3) says "the first on" sky, Haze and
  picture row.

`3d/include/3d/draw_system.h` (818 lines, a public header: not split;
open only lines 505-528): the comments of `voe_3d_draw_system_sky` and
`voe_3d_draw_system_picture` say the first row whose entity is on, an off
row (itself or through a parent) being as no row (0440). Signatures
unchanged.

`3d/src/src.md`: the `draw_system.c` entry, if it still says the first
rows, says the first on rows; one sentence under 300 characters.

New test `3d/tests/sky_switched_off.c`, needing no graphics card (both
functions take no device). Its worlds are built as `3d/tests/sky.c`'s
`a_world` builds them (read lines 1-120 and its `check_zeroed` and
`check_skyless`), plus the identity table (`scene/identity_system.h`) and
the parent table (`scene/parent_system.h`); switch an entity as
`3d/tests/lights_switched_off.c` does. Cases:
1. A Haze row on an entity with an identity: the record's `haze`,
   `haze_start` and `haze_top` are the row's; the entity off: all three 0;
   on: the row's again.
2. A Sky row likewise: on, `drawn` 1 and the row's colours; off, `drawn` 0
   and the colours 0, as with no row; on, back.
3. A Picture row of exposure -1 and Clip off: exposure the zeroed record;
   on: the row's.
4. A Sky and a Haze row on one entity, off: neither is in the record.
5. A Haze row on a child, its parent off: no haze; parent on: haze.
6. Two Haze rows (table capacity 2), the first's entity off: the second
   row's haze.
Its header lists the cases and names 0440. Entry in `3d/tests/tests.md`
beside `sky.c`'s, one sentence under 300 characters.

## Done when
The tests `3d/sky_switched_off` and `3d/sky` pass after the folder's
build. Then the human runs bug 10's How to test, steps 1-6
(`Agentic/kanban/089-a-model-has-lod-levels/bugs/10-switching-off-the-haze-leaves-the-haze.planned.md`),
in the editor on the hill project, and in Play for step 6; every step
holds.
