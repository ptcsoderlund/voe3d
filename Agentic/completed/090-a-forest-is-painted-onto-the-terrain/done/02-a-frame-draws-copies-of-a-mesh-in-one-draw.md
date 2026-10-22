# 02 — A frame draws copies of a mesh in one draw
folder: render
after: none
decisions: 0168, 0375, 0442

## Change
0442 point 6: instance records written once a frame and drawn by one instanced draw.

- `render/include/render/device.h` (public, 2300 lines: read only what you change; grep
  `voe_render_capacities`, `reserved0`, `voe_render_frame_draw_count`) —
  - `voe_render_capacities` gains `instances`, per frame slot, the seventh that may be nought:
    the first write on a device with none is refused with a message. Its paragraph above.
  - `voe_render_instance`: `voe_math_float4 rows[3]` (the top three rows of a camera-relative
    world matrix), `float dither`, three reserved words; 64 bytes.
  - `voe_render_object`'s `reserved0` becomes `uint32_t instances`, the first instance's
    index plus one, nought none; render sets it, a caller leaves it nought.
  - `[[nodiscard]] bool voe_render_frame_instances(device, const voe_render_instance *, uint32_t
    count, uint32_t *first)`: copied into the slot's instance buffer, `first` their index; false
    past the room, nothing written. Inside a frame; any time before the draws reading them.
  - `[[nodiscard]] bool voe_render_frame_draw_instanced(device, geometry, voe_render_object,
    uint32_t first, uint32_t count)`: one draw command of `count` instances, each placed by its
    instance's rows instead of `world`, its normal matrix the shader's (exact for a turn and a
    uniform scale), its dither instead of the record's. Solid pipeline in a camera pass, shadow
    pipeline in a shadow pass; asserts in a point-shadow, capture or bounce shadow pass, with a
    range past what was written, or count nought. Spends one object, counts one draw and
    index count ÷ 3 × count triangles. False as voe_render_frame_draw.
- `render/src/device_parts.h`, `render/src/descriptors.c` — a host-visible instance buffer per
  frame slot of `instances` records (one record when nought, for the binding), mapped as the
  objects' are; set 0 binding 12. `render/src/device_internal.h` — an instance count per frame,
  reset where `frame.c` resets `object_count`.
- `render/src/draw.c` — both calls; the instanced draw records through the same path as a
  draw, firstInstance 0. Header points: one record for all copies, why not in face passes.
- `render/src/records_layout.c` — size 64 and the offsets of `rows` and `dither`; `instances`
  at reserved0's offset.
- `render/shaders/bindings.slangh` — the instance struct, binding 12, the record's
  `instances`.
- `render/shaders/instances.slangh` — new part: from a record and SV_InstanceID, the world
  matrix, the normal matrix (the 3×3 over its first row's squared length) and the dither, the
  record's own when `instances` is nought. Header: what 0442 point 6 says.
- `render/shaders/draw.slang` (790 lines; keep it under 800, logic in the part) — the mesh
  vertex stage the solid, blended and shadow pipelines share takes SV_InstanceID and places
  through the part; it hands the dither to the fragment stage as a flat output, which the
  fragment reads instead of `object.dither`. The point-shadow and capture stages are unchanged.
- `render/tests/instances.c` — new, headless, its quad and read mirroring
  `render/tests/unshaded.c`: three instances of a small unlit white quad at three places, one
  draw: each place reads white and a spot between reads the clear colour; draw count 1,
  triangles three quads'; on a device of `instances` 3, a second write of one is refused and
  the frame still ends.
- `render/src/src.md`, `render/tests/tests.md`, `render/shaders/shaders.md`,
  `render/include/render/render.md` — entries follow.

## Done when
Tests `render/instances`, `render/dither`, `render/shadow` and `render/terrain_layers` pass.
