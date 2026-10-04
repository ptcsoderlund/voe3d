# 02 — Render knows which boxes hold a point
folder: render
after: none
decisions: 0168, 0347

## Change
The record of 0347 point 2 and the pure-CPU mask of point 3, no Vulkan, nothing drawn yet. Read
`render/src/light_bins.h` and `render/tests/light_bins.c` as the pattern for a pure-CPU part and its
test, and the `voe_render_point_light` section of `render/include/render/device.h`.

- `render/include/render/device.h`, after `voe_render_point_lights` (additions only):
  - `#define VOE_RENDER_LIGHT_BLOCKERS 32` and `#define VOE_RENDER_LIGHT_BLOCKER_PUSH 0.05f`.
  - `voe_render_light_blocker { voe_math_float4 rows[3]; voe_math_float4 sphere; }`, 64 bytes, a
    static_assert says so.
  - `voe_render_light_blockers { const voe_render_light_blocker *blockers; uint32_t count; }`.
  - Comment points: rows take a position in the pass's space (about the eye) to the box's unit space,
    so for a box of centre c, unit axes a_i and half sizes h_i row i is (a_i / h_i, −a_i·c / h_i);
    inside when every |row.xyz·p + row.w| ≤ 1; the sphere bounds the box and is tested first; a
    point's mask is bit i for blocker i holding it; what the masks gate (0347 points 3–4, one line
    each); a surface is tested pushed PUSH along its normal and why; nought blockers is the old
    picture; at most VOE_RENDER_LIGHT_BLOCKERS.
- `render/src/light_blockers.h` (new), header comment with a usage block like light_bins.h's:
  `uint32_t voe_render_light_blockers_mask(const voe_render_light_blocker *blockers, uint32_t count,
  voe_math_float3 point)`: the sphere test then the rows, inclusive at the face. Asserts count ≤
  VOE_RENDER_LIGHT_BLOCKERS. Header point: draw.slang and the relight test a point the same way.
- `render/src/light_blockers.c` (new): the call.
- `render/tests/light_blockers.c` (new), including `../src/light_blockers.h`: an unrotated box of
  half size 1 about (0, 0, −5) holds its centre and a point on its face, not one 1.01 m out; a box
  turned 45° about Y holds a point along its diagonal inside and not one just past its edge; a point
  in two boxes has both bits; blocker 31 sets bit 31; nought blockers is mask 0; a point inside the
  sphere and outside the box is 0.
- `render/src/src.md`, `render/tests/tests.md`: one entry each for the new files; the device.h entry
  in `render/include/render/render.md` names light blockers. Each at most 300 characters.

## Done when
The test `render/light_blockers` passes after the folder's build.
