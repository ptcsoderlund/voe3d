# 01 — A downloaded fir tree does not import

## Seen
I cannot import a glb file. It blocks step 1 of 090's test: I have no tree to paint.

```
error: assets: glTF: a count or an index that is not a whole number inside its range
error: assets: glTF: a count or an index that is not a whole number inside its range
error: assets: glTF: a count or an index that is not a whole number inside its range
error: editor: Assets/Lanscapestuff/fir_tree_01_4k.glb: its levels were not made: the data is not what it claims to be
error: assets: glTF: a count or an index that is not a whole number inside its range
error: assets: glTF: a count or an index that is not a whole number inside its range
error: 3d: could not load the model Assets/Lanscapestuff/fir_tree_01_4k.glb
error: game: could not read the model Assets/Lanscapestuff/fir_tree_01_4k.glb: the data is not what it claims to be
```

The file is a downloaded fir tree (Poly Haven's `fir_tree_01`, 4k), opened in Blender and exported as
`.glb` by Blender's own glTF exporter 5.2.40. It is a well-formed glTF 2.0 file, but large:

- 499 MB; one 499 MB buffer; one mesh, one node, four primitives, four materials.
- The big primitive has 5,368,447 vertices and 12,198,357 indices (about 4 million triangles), with
  32-bit indices. The others have 12,892, 44,259 and 3,325 vertices, with 16-bit indices.
- Every primitive has `COLOR_0` (normalized 8-bit VEC4) and `COLOR_1` (normalized 16-bit VEC4).
  One primitive has no `TEXCOORD_0`.
- Six embedded 4k PNGs, the largest bufferView 85 MB.
- `KHR_texture_transform` is in `extensionsRequired`; `KHR_materials_specular` and `KHR_materials_ior`
  are used.

The file lives at `~/Projekt/voe3dprojects/lumberhill/lumberhill_project/Assets/Lanscapestuff/fir_tree_01_4k.glb`
(a copy in `~/Hämtningar/fir_tree_01_4k.blend/`).

## Expected
A valid `.glb` from Blender or a download imports (0280: no refusing; 0376: models from Blender or
downloaded), however big, as long as the machine can hold it. Its LODs are made, it can be placed, and it
can be picked in the foliage brush; the far trees of a forest draw its simple LODs.

The error should have said "too many vertices" or something that makes sense to an artist trying to
import. From now on every import, load and LOD error follows 0443: the file, the part, what is wrong in
an artist's terms with the numbers, what to do in Blender; a limit the engine keeps is called a limit,
not bad data; one line per cause, not five copies. So if a limit stays that this tree still exceeds,
importing it says so in those words. Today's import, load and LOD messages are brought to 0443 as part of
this bug, not only the one this tree hit.

## How to reproduce
1. Open the lumberhill project.
2. Import `fir_tree_01_4k.glb` (or copy it into `Assets/`).
3. The errors above appear; the model cannot be placed or picked for the foliage brush.
