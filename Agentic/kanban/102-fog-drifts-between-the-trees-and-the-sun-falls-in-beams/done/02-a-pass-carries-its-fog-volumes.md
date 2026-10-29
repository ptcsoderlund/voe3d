# 02 — A pass carries its fog volumes
folder: render
after: none
decisions: 0168, 0480, 0493

## Change
0493 point 3: the record reaches the pass's block; nothing reads it yet and no picture changes.
Card 02 of 101 (`Agentic/completed/101-clouds-in-the-sky-cast-shadows-on-the-world/done/02-a-pass-carries-its-clouds.md`)
is the shape. Read in `render/include/render/device.h` the `voe_render_light_blocker` comment and
struct (the rows), the `voe_render_clouds` comment and struct, `voe_render_pass_camera` and its
comment, and the pass-begin assert paragraph; in `render/src/device_parts.h` the frame block and
its comment; in `render/shaders/bindings.slangh` the clouds struct and the frame block;
`render/src/records_layout.c`; the header of `render/src/pass_sky.c`.

- `render/include/render/device.h`: `VOE_RENDER_FOG_VOLUMES` 8, `VOE_RENDER_FOG_PATCHES` 128.0f,
  `VOE_RENDER_FOG_CHANGE` 32.0f; `voe_render_fog_volume` (`voe_math_float4 rows[3]`,
  `voe_math_float3 patches`, `float density`, `voe_math_float3 change`, `float patchiness`,
  `float godrays`, `uint32_t reserved[3]`), static-asserted 96 bytes; `voe_render_fog`
  (`voe_render_fog_volume volumes[VOE_RENDER_FOG_VOLUMES]`, `uint32_t count`,
  `uint32_t reserved[3]`), static-asserted. Comment points: zeroed is none, the old picture byte
  for byte; the rows as a light blocker's, about the eye to the box's unit space; each field as
  0493 point 3, `density` the extinction per metre in the thickest place, `patches` and `change`
  the eye's place in fields that repeat every PATCHES and CHANGE metres; the fade, shape and
  lighting are said by the calls that draw it. `voe_render_pass_camera` gains `fog` after
  `clouds`, its comment one sentence. The pass-begin assert paragraph adds: a count past
  VOE_RENDER_FOG_VOLUMES; in a counted record a number not finite, density below 0, patchiness or
  godrays outside 0 to 1, patches outside 0 to PATCHES or change outside 0 to CHANGE.
- `render/src/device_parts.h`: the frame block gains `voe_render_fog fog` after
  `clouds_reserved`; the comment says it is the camera's, zeroed as the clouds are.
- `render/shaders/bindings.slangh`: the matching structs and block member, with a comment as the
  clouds'.
- `render/src/records_layout.c`: the size of both records and the offsets of their members and
  of the block's new member asserted, as the clouds' are.
- `render/src/pass_sky.c`: copies the camera's fog into the block, zeroed with no camera and in
  shadow, point-shadow and capture passes, asserting the bounds above; the header names the fog.
- `render/src/src.md` (`pass_sky.c`) and `render/shaders/shaders.md` (`bindings.slangh`): follow,
  one phrase each, each entry at most 300 characters.

## Done when
The tests `render/sky`, `render/clouds` and `render/exposure` pass after the folder's build.
