# 04 — A lit surface reads the sun's cascades
folder: render
decisions: 0168, 0258, 0238

## Change
The camera pass samples card 03's maps (0258 point 3). A zeroed record is no shadow, so every
existing `{ view, light }` initializer keeps building and drawing as before.

- `render/include/render/device.h`:
  - `voe_render_shadow` (new): `voe_math_float4x4 cascades[VOE_RENDER_SHADOW_CASCADES]` —
    eye-relative world to each cascade's clip space, the same view × projection its shadow pass
    drew with; `float splits[VOE_RENDER_SHADOW_CASCADES]` — the far view distance of each;
    `float texels[VOE_RENDER_SHADOW_CASCADES]` — metres one texel covers in each, for the
    normal offset; `uint32_t count` — cascades in use, nought for none; padded like the other
    blocks. Paragraph:
    whose numbers these are (3d's), nought is none, what a surface does with them.
  - `voe_render_pass_camera` gains `voe_render_shadow shadow` as its third member.
  - The sun paragraph's "there is no shadow" rewritten: the sun casts through the pass's shadow
    record (0258); `unshaded` still wins.
- `render/src/device_parts.h` — `struct voe_render_frame_block` gains the shadow record.
- `render/src/descriptors.c` — binding 5: the slot's shadow array view with a comparison
  sampler (compare op for reversed depth, LINEAR, CLAMP_TO_BORDER to the lit side); the block's
  size asserts; header: six bindings, binding 5's line, why one per slot.
- `render/src/shadow.c` — makes and destroys the comparison sampler.
- `render/src/pass.c` — `voe_render_pass_begin` copies the shadow record into the block.
- `render/shaders/draw.slang` — the block mirrors the C record; binding 5 declared; for a lit
  surface in a shaded pass with `count` > 0: cascade by view depth against `splits`, the
  position offset along the normal by a texel's worth, projected, uv from clip with the one Y
  flip (clip +y is row 0), a 3×3 `SampleCmpLevelZero` average, beyond the last split fully lit;
  the sun's direct term times it; emissive and unlit untouched. Header's lighting section: "no
  shadows" becomes the cascades, the offset and the filter, and why shadowed is as dark as
  facing away.
- `render/shaders/shaders.md` — `draw.slang`'s entry: shadows by cascades.
- `render/tests/shadow.c` — add, headless: a floor of a flattened cube and a cube above it,
  sun straight down, one shadow pass onto cascade 0 with a hand-built orthographic light view
  over both, then a window pass with `count` 1 and a split past the scene: a floor pixel under
  the cube reads darker than a floor pixel beside it and the cube's top reads as lit; the same
  frame with `count` 0 reads both floor pixels alike; with `unshaded` set both read the base
  colour.
- `render/tests/tests.md` — `shadow.c`'s entry grows.

## Done when
1. `bash ~/.claude/skills/checks/scripts/checks.sh --folder render` prints `FINDINGS: 0`.
2. `ctest --test-dir build/debug -R '^render/'` passes (`shadow`, `unshaded`, `offscreen`
   among them).
