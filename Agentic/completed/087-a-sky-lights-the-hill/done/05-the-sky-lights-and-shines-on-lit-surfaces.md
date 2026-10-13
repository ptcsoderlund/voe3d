# 05 — The sky lights and shines on lit surfaces
folder: render
after: 04
decisions: 0168, 0405

## Change
0405 point 5. Read the headers of the files below, `render/src/sky.h`, and `render/tests/unshaded.c` and
`render/tests/blocker_kinds.c` as patterns.

- `render/src/device_parts.h`: `struct voe_render_frame_block` gains `voe_math_float4 sky_harmonics[9]`
  after the sky record; `render/shaders/bindings.slangh` its twin; `render/src/records_layout.c` /
  `render/src/descriptors.c` check it.
- `render/src/pass.c`: a camera pass whose sky `light` is above nought fills the nine by
  `voe_render_sky_harmonics` from its light's direction and colour × intensity (nought for an
  `unshaded` light); otherwise zero. Header names it.
- `render/shaders/lighting.slangh`: includes `sky.slangh`; on the lit path, a sky term added beside
  the indirect term: diffuse `(1 − metal) × base × E(n)/π × occlusion × light` with E from the nine,
  and reflection `lerp(sky(R), E(R)/π, roughness) × EnvBRDFApprox(F0, roughness, N·V) × occlusion ×
  light` (Karis's analytic fit), R the reflected view. Both only when the surface's blocker mask
  holds no bit outside the walls' mask; light nought adds exactly nothing. The water path is
  untouched. Header gains a paragraph for the sky term.
- `render/include/render/device.h`: the `voe_render_sky` comment says what `light` does and that it
  is gated as above.
- `render/tests/sky_light.c` (new, headless): a grey lit cube, sun overhead turned away from the face
  read, sky light 1: the face lit, blue above red; light 0: black, as before; the sun 5° up on the
  face's side: the face's red over blue above noon's; a smooth white metal quad tilted to the
  camera reads the sRGB of `voe_render_sky_radiance` along R times its Fresnel within 6/255, and
  rough at 1 reads nearer E(R)/π than the sky; the cube inside an Indoors blocker reads black.
- `render/src/src.md`, `render/shaders/shaders.md`, `render/tests/tests.md`: entries changed where
  what they say changed, one new for the test. Each at most 300 characters.

## Done when
The tests `render/sky_light`, `render/sky`, `render/unshaded`, `render/blocker_kinds` and
`render/directional_lights` pass after the folder's build.
