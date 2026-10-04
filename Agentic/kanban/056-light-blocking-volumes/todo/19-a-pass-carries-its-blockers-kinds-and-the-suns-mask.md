# 19 — A pass carries its blockers' kinds and the sun's mask
folder: render
after: 18
decisions: 0168, 0348, 0350

## Change
The C side of 0350 point 2 for a camera pass; no shader reads the new words yet (card 21). Read
the headers of `render/src/pass.c`, `render/src/light_blockers.h`, `render/src/light_blockers.c`,
`render/src/device_parts.h`, `render/src/descriptors.c`, `render/shaders/bindings.slangh`, and the
light blocker and `voe_render_pass_camera` sections of `render/include/render/device.h`.

- `render/include/render/device.h`:
  - `voe_render_light_blockers` gains `uint32_t walls`, `uint32_t indoors`, `uint32_t sun`.
    Comment points: bit i is blocker i; a blocker in neither is a Room; `sun` is the mask of the
    blockers holding the directional light's place, from the call below; all zero is 0347's
    picture; walls and indoors sharing a bit, or any bit at or past `count` in the three, asserts
    at pass begin.
  - The comment over `voe_render_light_blocker` states 0350 points 3–5 in place of 0347's
    mask-equality lines (one line each), keeping the rows, sphere and push points.
  - `uint32_t voe_render_light_blockers_mask(const voe_render_light_blocker *blockers, uint32_t
    count, voe_math_float3 point)` declared here, moved from `render/src/light_blockers.h`, with
    its header's points as its comment; the caller named is 3d, for the sun's mask.
  - `voe_render_pass_camera`'s `blockers` comment names the kinds.
- `render/src/light_blockers.h`: deleted. `render/src/light_blockers.c`, `render/src/pass.c` and
  `render/tests/light_blockers.c` include `render/device.h` for the call instead.
- `render/src/device_parts.h`: `struct voe_render_frame_blockers` gains, after the masks,
  `walls`, `indoors`, `sun` and one reserved word; its comment says what they are.
- `render/src/descriptors.c`: the region's size assert matches.
- `render/src/pass.c`: `place_blockers` writes the three words (zero with no blockers) and asserts
  as the device.h comment says; header point.
- `render/shaders/bindings.slangh`: `voe_render_blocker_region` matches, the three words named.
- `render/tests/light_blockers.c`: a case that the call is reachable through `render/device.h`.
- `render/src/src.md`, `render/tests/tests.md`, `render/include/render/render.md`,
  `render/shaders/shaders.md`: light_blockers.h's entry gone; the entries for what changed. Each
  at most 300 characters.

## Done when
The tests `render/light_blockers`, `render/blocked_light` and `render/passes` pass after the
folder's build, and `test ! -e render/src/light_blockers.h` exits 0.
