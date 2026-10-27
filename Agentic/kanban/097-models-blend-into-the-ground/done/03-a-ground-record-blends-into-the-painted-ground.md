# 03 — A ground record blends into the painted ground
folder: render
after: 02
decisions: 0168, 0481, 0402

## Change
0481 point 1 in the mesh fragment stage.

- `render/shaders/ground_blend.slangh` — new part, included by `draw.slang` after
  `terrain_layers.slangh`. One function taking the object, the pixel's camera-relative position
  and the model's surface (base colour, ORM, normal, lit) and returning it blended:
  - landscape metres p = `ground_rows` × (position, 1); ground height h by
    `voe_render_terrain_height(ground − 1, terrain.w, p.xz)`; above = p.y − h;
    t = 1 below the ground, else smoothstep(0, 1, 1 − above / `ground_reach`); a pixel past the
    landscape's square (|x| or |z| over side / 2), or a reach not above nought, has t nought.
  - the layers' surface at uv = p.xz / side + 0.5 through `terrain_layers.slangh`; base colour
    lerped to its colour by t; when lit the ORM likewise, and the normal lerped toward the
    ground's world normal: the central-difference normal one texel step (side / (w − 1)) apart,
    its layer normal texel applied as the landscape's own fragment path applies it, turned to
    world by the transpose of `ground_rows`' 3 × 3.
  - Header: the claim and formula, that it is read only when `ground` and `paint[0]` are not
    nought, its cost (four height reads, the layers' reads, per pixel under the reach), and the
    control-flow rule below.
- `render/shaders/terrain_layers.slangh` — `voe_render_layer_surface_at` takes its derivatives
  before any branch. Add `voe_render_layer_surface_grad(object, uv, dx, dy, colour_only)` taking
  them, `_at` calling it, so the blend's read can sit in a per-pixel `t > 0` branch with the
  gradients taken in uniform flow before it. Every read inside that branch must take no
  implicit derivative (paint and height reads by level or Load); fix any that does. Its header
  says so.
- `render/shaders/draw.slang` — the include, and the main fragment entry calls the blend when the
  record's `ground` and `paint[0]` are not nought (a branch uniform per draw), after the model's
  surface is known and before alpha and lighting; alpha, cut-out and dither untouched. The
  capture and point-shadow entries do not call it. Its header's list names the blend.
- `render/shaders/shaders.md` — an entry for the part; `draw.slang`'s and `terrain_layers`'
  entries follow.
- `render/tests/ground_blend.c` — new, built as `render/tests/terrain_layers.c` is (read it for
  the painted landscape's records and the orthographic camera from above): a flat 200 m
  landscape, heights nought, painted all layer 0, an unlit red record (its textures needed, its
  nodes need not be drawn); an unlit white square that fills the picture, its record a ground record of that landscape with
  reach 1 m. Claims: A, the square 0.1 m above the ground draws red (t ≈ 0.97), red over 230,
  green and blue under 30; B, at 2 m it draws white; C, at 0.1 m with `ground` nought it draws
  white; D, at 0.5 m every channel between A's and B's. Skips without usable Vulkan, as the
  others do. Its header states the claims.
- `render/tests/tests.md` — an entry for `ground_blend.c`.

## Done when
`ctest --test-dir build/debug -R '^render/ground_blend$'` passes, and the existing render tests
still pass.
