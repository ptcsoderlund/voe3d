# 67 — The Inspector counts LODs from one
folder: editor
after: none
decisions: 0168, 0433, 0434

## Change
0434: the Inspector's LOD rows show the level drawn plus one; the count after "of" and every
name, index and file stay as they are. Labels keep their length, so no budget changes.

- `editor/src/inspector_level.c`: the row "View <n>: LOD <k> of <count>, made|own" (line ~96)
  shows k + 1; a model without levels (line ~92) reads "View <n>: LOD 1 of 1". The file's
  header comment (lines 4-5) says counted from one, the full model LOD 1 (0434), so no levels
  reads "LOD 1 of 1" and the farthest of six "LOD 6 of 6".
- `editor/src/inspector_level.h`: the header (lines 3-4) says k counted from one, the full
  model LOD 1, and "View <n>: LOD 1 of 1" for a model without levels, naming 0434.

## Done when
`voe_editor` builds in the folder's check with no warning,
`grep -c 'LOD 1 of 1' editor/src/inspector_level.c` prints at least 1 and
`grep -c 'LOD 0' editor/src/inspector_level.c editor/src/inspector_level.h` prints 0 for each.
The human runs bug 07's How to test
(`Agentic/kanban/089-a-model-has-lod-levels/bugs/07-the-inspector-should-count-lods-from-one.planned.md`):
1. Make LODs on the stable with 6; a placed stable close by says LOD 1 of 6; flying slowly away
   it says 2, 3, … up to LOD 6 of 6, and the Frame panel's Triangles drops at each.
2. Choose 2 on a model: the Inspector goes no higher than LOD 2 of 2.
3. A model with no LODs says LOD 1 of 1.
And the feature's How to test, steps 1 to 6, still hold.
