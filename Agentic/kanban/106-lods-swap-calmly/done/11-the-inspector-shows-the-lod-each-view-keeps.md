# 11 — The Inspector shows the LOD each view keeps
folder: editor
after: 08, 10
decisions: 0168, 0500, 0501

## Change
The Inspector's LOD section asks the stateless `voe_3d_model_level`, which no longer is what a view
draws: a copy creeping back keeps the coarser LOD. It reads the view's memory instead
(`3d/include/3d/lods_kept.h`; read its header; card 10 gave each view its `lods`).

- `editor/src/inspector_level.c`: per view in use (line ~86), the LOD is the `level` of
  `voe_3d_lods_kept_find(view->lods, voe_3d_lods_kept_row_key(entity), ...)`; when that is false
  (not drawn yet, or no memory) it falls back to `voe_3d_model_level` as now. Labels unchanged.
- `editor/src/inspector_level.h`: the header comment says it shows the LOD each view keeps, as its
  draws chose it last, falling back to the distance's LOD.
- `editor/src/src.md`: the `inspector_level` entry follows.

## Meaning
- Holds: hysteresis per view; a copy creeping back inside the 10% shows the coarser LOD whole
  (0495 pt 2, 0500 pt 1).

## Done when
`grep -q voe_3d_lods_kept_find editor/src/inspector_level.c` exits 0, and the editor builds.

The human's, the feature's How to test:
1. A tree's "LODs of <file>", swaps ×0.2, ×0.5, ×0.5, ×1, ×0.8: each LOD's Swap distance farther
   than the one above by at least a quarter; no two the same.
2. In the editor, one tree selected, its LOD in the Inspector: fly slowly away until it swaps; creep
   back a little, it keeps the coarser LOD; about a tenth nearer it swaps back, dissolving.
3. Hover near a swap and nudge back and forth: no flicker. The same at the painted forest's edge:
   no shimmer.
4. Both scene views on the forest from different places: each view's trees swap by its own camera.
5. Play, the same path: the same calm swaps. Ship and run the build: the same.
6. A project saved before this: models look as before, except a LOD once hidden by overlapping
   swaps now shows over its own stretch.
