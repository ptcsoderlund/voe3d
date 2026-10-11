# 03 — A painted landscape blends its layers by height, hex-tiled
folder: render
after: 02
decisions: 0168, 0402

## Change
0402 points 5 and 6: the fragment stage of a terrain record with paint shades it from its layers'
records instead of its own.

- `render/shaders/terrain_layers.slangh` — new part, its header saying what 0402 points 5 and 6
  say, the cost (three taps a map, per layer present) and why no rotation. Included by
  `draw.slang` after `terrain.slangh`. Holds:
  - a hex-tiled read: `uv`, a texture index, the unoffset uv's gradients; the three nearest hex
    cells' hashed offsets, each sampled with those gradients (`SampleGrad`), blended by barycentric
    weight sharpened by the sample's luminance, normalised.
  - the paint's weights at landscape uv: both paint textures (`object.paint_a`/`_b` minus one)
    sampled bilinear with texel centres on the height points, uv' = (uv·(w − 1) + 0.5) / w.
  - the layer surface: for each of the eight layers with weight above 1/255, its record from
    `layers_low`/`layers_high`, its maps at landscape metres (uv × `object.terrain.w`) × the
    record's `uv_repeat` (nought as one); height = ORM (`occlusion_texture`) red when it has one,
    else the base colour's luminance; s = height + weight; blend by max(0, s − (max s − 0.2)),
    normalised; then for layers whose blend is above nought, base colour (texture × factor),
    tangent-space normal texel and ORM (or the record's roughness and metallic when it has none,
    0401) are blended. Returns a struct of base colour, normal texel (or none), occlusion,
    roughness, metallic.
- `render/shaders/draw.slang` — in `voe_render_draw_fragment` and `voe_render_capture_fragment`,
  where the shading record's base colour, normal map, ORM, roughness and metal are read: when the
  object's `heights` and `paint_a` are not nought, take them from the layer surface; alpha and
  lit/unlit stay the object's own record and colour, so a fading landscape fades whole. The
  capture fragment takes the blended colour only. The parts paragraph of the header names the
  new part; keep the file under ~700 lines by putting logic in the part.
- `render/shaders/shaders.md` — entry for `terrain_layers.slangh`; `draw.slang`'s entry mentions
  the layers.
- `render/tests/terrain_layers.c` — new, headless, after `render/tests/terrain_vertex.c`'s
  record: a flat 2-cell heights texture viewed straight down, an unlit object record (white),
  two unlit layer records red and green with no maps; paint textures 3 × 3 with the left column
  all layer 0 and the right column all layer 1. The picture's left quarter reads red, right
  quarter green, nothing else; with `paint_a` nought the whole reads the object record's white;
  with a layer at weight 0 everywhere its colour shows nowhere.
- `render/tests/tests.md` — entry.

Step 4 of How to test (no tiled pattern seen from high up) is the human's, at card 11.

## Done when
Tests `render/terrain_layers` and `render/terrain_vertex` pass.
