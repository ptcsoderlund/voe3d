# 04 — The shapes' upload hands back an unlit record too
folder: 3d
decisions: 0168, 0189, 0191, 0203

## Change
The outline's quads need a record that is not lit — a flat line that the sun shades is a line whose colour
changes as the camera moves, and 0194 says what is drawn for state carries the theme's lightness and nothing
else. The shapes' upload is where a record the built-in shapes need is made, so it makes this one too.

`3d/include/3d/shape_system.h`:

- `VOE_3D_SHAPES_SHADINGS` becomes 2, and the sentence above it that says what the constants size a device
  with names the second record.
- `voe_3d_shapes` gains `voe_3d_material outline;` under `material`, with a comment saying it is the record the
  selection outline's quads wear (0203): white, opaque and **unlit**, so the colour in the drawn object's
  record is the whole of what they are (ADR-0191's rule for a shape's colour, applied to a line), and that
  nothing in this folder adds it to an entity — it is handed to a pass through `voe_3d_frame` (card 05).
- `voe_3d_shapes_upload`'s comment says it uploads two records now, in the order it makes them.

`3d/src/shape_system.c`: after the white lit material, upload a second `voe_3d_material` — base colour white
and fully opaque, metallic 0, roughness 1, `unlit` true, no textures — through `voe_3d_material_upload`, into
`out->outline`. A failure is the same returned failure the first upload's already is, with the same error left
as `voe_3d_material_upload` set it.

`3d/tests/shape.c`, in the half that has a device: after an upload, `shapes.outline.shading` is not the same
id as `shapes.material.shading`, and `shapes.outline.unlit` is true while `shapes.material.unlit` is false.

`3d/src/src.md`'s line for `shape_system.c` says it uploads the shapes' one white material and the outline's
unlit one; `3d/tests/tests.md`'s line for `shape.c` says the upload half checks both records.

## Done when
`checks.sh 3d` exits 0, with `3d/shape`'s device half running on this machine's card and the two new checks in
it passing.
