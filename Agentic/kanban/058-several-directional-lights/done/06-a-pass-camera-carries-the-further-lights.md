# 06 — A pass camera carries the further lights
folder: render
after: 05
decisions: 0168, 0357, 0350

## Change
Decision 0357 point 1, carried to the frame block. The shader declares the new members but does
not read them yet (card 07). Files:

- `render/include/render/device.h`, after `voe_render_shadow`:
  - `voe_render_directional_light`: `{ voe_render_light light; voe_render_shadow shadow; uint32_t
    blockers; uint32_t bounces; float bounce_strength; uint32_t reserved; }`. Comment points:
    - `blockers` is the mask of the pass's blockers holding this light's place, as the blockers'
      `sun` is the first light's;
    - `bounces` and `bounce_strength` are read by a bounce begin (card 08), not by a pass;
    - a `shadow` with `count` reads its own `slot`.
  - `voe_render_directional_lights`: `{ const voe_render_directional_light *lights; uint32_t
    count; }`. NULL with a count, or a count above `VOE_RENDER_DIRECTIONAL_LIGHTS − 1`, asserts.
  - `voe_render_pass_camera` gains `voe_render_directional_lights more;` last. Comment: the lights
    after the first; zero is the old picture, byte for byte.
- `render/src/device_parts.h`: after `struct voe_render_frame_block`'s `bounce`, add:
  - `uint32_t more_count`, three reserved words;
  - `struct voe_render_frame_light more[VOE_RENDER_DIRECTIONAL_LIGHTS − 1]`. The new struct is
    `{ voe_render_light light; voe_render_shadow shadow; uint32_t blockers; uint32_t reserved[3]; }`.

  Update the block's comment.
- `render/src/records_layout.c`: the asserts for `voe_render_directional_light` (368 bytes), the
  frame light (368), the block's new size, and the offsets of `more_count` (544) and `more` (560).
  Every existing assert stays.
- `render/src/pass.c`: a camera pass writes `more_count` and each entry. Each entry's `blockers` is
  masked to the pass's blocker count, as the first light's `sun` is. Assert, as the first light's
  are checked:
  - every light finite;
  - a shadowed entry's `slot` below the lights ready;
  - no two shadowed lights, the first included, on one slot.

  A shadow pass, and every other pass kind, writes `more_count` nought. Header: one point on the
  further lights.
- `render/shaders/bindings.slangh`: the same members in the frame block struct, in the same order,
  and the light struct. Read its header; nothing reads them yet.
- `render/src/src.md` and `render/shaders/shaders.md`: the `pass.c` and `bindings.slangh` entries
  name the further lights.
- `render/tests/passes.c` (read its header for the pattern): one case where a camera with two
  further lights, one shadowed on slot 1, opens, draws and ends.

## Done when
`ctest --test-dir build/debug -R "^render/(passes|shadow_lights|shadow)$"` passes after the render
build.
