# 02 — A pass carries its clouds
folder: render
after: none
decisions: 0168, 0480, 0492

## Change
0492 point 3: the record reaches the pass's block; nothing reads it yet and no picture changes.
Card 63 of 087 (`Agentic/completed/087-a-sky-lights-the-hill/done/63-the-sky-record-holds-the-haze-s-top.md`)
is the shape. Read in `render/include/render/device.h` the `voe_render_sky` comment and struct,
`voe_render_pass_camera` and its comment, and the pass-begin assert paragraph; in
`render/src/device_parts.h` the frame block and its comment; in `render/shaders/bindings.slangh`
the sky struct and the frame block; `render/src/records_layout.c`; the header of
`render/src/pass_sky.c` and its entry in `render/src/device_calls.h`.

- `render/include/render/device.h`: `VOE_RENDER_CLOUDS_NONE` 0, `_FLAT` 1, `_VOLUMETRIC` 2,
  `VOE_RENDER_CLOUDS_PERIOD` 8192.0f; `voe_render_clouds` (`uint32_t kind`, `float coverage`,
  `float bottom`, `float thickness`, `voe_math_float2 origin`, `float shadow_strength`,
  `uint32_t reserved`), static-asserted 32 bytes. Comment points: zeroed is none, the old picture
  byte for byte; each field as 0492 point 3, `bottom` above the eye as `haze_top` is, `origin` the
  eye's place in a field that repeats every PERIOD on x and z; what is drawn, shadowed and
  mirrored is said by the calls that do it. `voe_render_pass_camera` gains `clouds` after
  `exposure`, its comment one sentence. The pass-begin assert paragraph adds: kind past
  VOLUMETRIC, coverage or shadow strength outside 0 to 1, thickness below 0, bottom not finite,
  origin outside 0 to PERIOD.
- `render/src/device_parts.h`: the frame block gains, after `sky_harmonics`, `voe_render_clouds
  clouds`, `uint32_t clouds_image` (the pass target's clouds image texture slot, render's own,
  `~0u` until a later card names it) and three reserved words; the comment says both.
- `render/shaders/bindings.slangh`: the matching struct `voe_render_clouds` and block members,
  with a comment as the sky's.
- `render/src/records_layout.c`: size of the clouds record and the offsets of its members and of
  the block's two new members asserted, as the sky's are.
- `render/src/pass_sky.c`: copies the camera's clouds into the block, zeroed with no camera and
  in shadow, point-shadow and capture passes, asserting the bounds above; writes `clouds_image`
  `~0u`; the header names the clouds. Its `device_calls.h` entry says so if it lists fields.
- `render/src/src.md` (`pass_sky.c`) and `render/shaders/shaders.md` (`bindings.slangh`): follow,
  one sentence each of at most 300 characters.

## Done when
The tests `render/sky`, `render/haze` and `render/exposure` pass after the folder's build.
