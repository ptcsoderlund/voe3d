# 52 — A metal-roughness map alone gives roughness and metal
folder: render
after: none
decisions: 0168, 0401

## Change
0401: where a shading record names a metallic-roughness texture, the surface's roughness is the
texture's green and its metal the texture's blue, as stored; the record's `roughness` and `metallic`
are not read. With `VOE_RENDER_NO_TEXTURE` (0) in that slot the factors are the values, as now. This
one change serves a `.material`'s ORM map, the game's cooked materials and a glTF model's
metallic-roughness texture alike; fixed once here, not per caller.

- `render/shaders/draw.slang` — in the mesh fragment stage, the two lines that set `roughness` and
  `metallic` as factor times `orm.g` / `orm.b`: choose instead between the map's channel and the
  factor by whether `shading.metallic_roughness_texture` is 0, each still saturated. A select on a
  record value after every Sample, not a fifth uniform branch (the header's watch on branches is
  untouched). The comment above `orm` gains that the map replaces the factors (0401), and that a
  non-zero id naming no live texture samples white and so reads rough and fully metal. Line count
  within a few of today's.
- `render/shaders/bindings.slangh` — the comment by `metallic` / `roughness` in the shading record, only
  if it says they multiply a map; it must agree.
- `render/include/render/device.h` — in `voe_render_shading_values`, the comment on the five texture
  ids gains that a metallic-roughness texture's green and blue replace `roughness` and `metallic`
  (0401); the `metallic` / `roughness` fields' comments likewise if they say "times".
- `render/src/texture.c` — the white default's comment says a missing roughness map multiplies its
  factor by one; that now holds for the base colour only, the metal-roughness slot being read as
  empty by its id (0401).
- `render/shaders/shaders.md` — `draw.slang`'s line gains that a metal-roughness map replaces the
  factors.

Test: add cases to `render/tests/surface_maps.c` (read its header; it already builds one-pixel DATA
textures), each lit by the reference's sun from +Z with no fill, base colour white:
- 11: roughness 1, metallic 0, no metal-roughness texture: the matte reference;
- 12: roughness 0.1, metallic 0, a metal-roughness texel green 255 blue 0: equal to case 11 within
  the tolerance, the shiny factor ignored;
- 13: roughness 1, metallic 1, no texture: the metal reference;
- 14: roughness 1, metallic 0, a texel green 255 blue 255: equal to case 13 within the tolerance.
Before writing 12 and 14 check that roughness 0.1 alone (no texture) differs from case 11, and that
case 13 differs from case 11, by more than the tolerance in some channel; if either does not, pick
the sun or base so it does, and say so in the header. Update the header's case list, `CASES` and the
capacities; `render/tests/tests.md`'s `surface_maps.c` line gains the metal-roughness map replacing
the factors.

## Done when
`ctest --test-dir build/debug -R '^render/surface_maps$'` passes with cases 11–14.
