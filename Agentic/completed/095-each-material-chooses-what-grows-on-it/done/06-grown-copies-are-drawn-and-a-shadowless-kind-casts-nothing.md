# 06 — Grown copies are drawn, and a shadowless kind casts nothing
folder: 3d
after: 05
decisions: 0168, 0442, 0450, 0464, 0476

## Change
0476 points 5 and 6 in the foliage gather: a row's grown copies are drawn as its own are, and a
kind whose look is `shadowless` is drawn in the camera pass only. The cascades and the bounce
sun maps both draw foliage through `voe_3d_draw_foliage_cast`, so they need no change.

- `3d/include/3d/draw_system.h` — `voe_3d_foliage_group` gains `bool casts`. The
  `voe_3d_draw_system_foliage` comment: each row's grown copies
  (`voe_3d_models_landscape_grown`, models_foliage.h) are gathered after its own, the same way;
  a copy of a shadowless kind is seen or left out, never cast or unseen, and counted as such,
  in groups whose `casts` is false.
- `3d/src/draw_foliage.h` — `struct voe_3d_foliage_key` gains `casts`; `_solid` draws every seen
  group as now; `_cast` skips groups whose `casts` is false. A header paragraph, A SHADOWLESS
  KIND, says why its copies are kept out of the cast and unseen tiers (nothing but the shadow
  passes would draw them) and that they are still shadowed.
- `3d/src/draw_foliage.c` — both walks walk a row's copies and then its grown copies with their
  placed transforms, in the same order; a kind's `shadowless` read once a row with its parts;
  the header's two-walks paragraph names the grown copies.
- `3d/tests/draw_foliage.c` — new cases: a landscape row with hand copies and grown copies of one
  kind gathers groups whose counts sum to both, per part; with that kind's look shadowless,
  every group is seen with `casts` false and the cast and unseen tiers hold none of its copies.
- `3d/src/src.md`, `3d/3d.md` — the `draw_foliage.*` and `draw_system.h` entries say grown
  copies and shadowless kinds.

## Done when
The `3d/draw_foliage` test passes with the new cases.
