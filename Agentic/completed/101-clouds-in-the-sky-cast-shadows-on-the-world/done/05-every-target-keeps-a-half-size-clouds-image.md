# 05 — Every target keeps a half-size clouds image
folder: render
after: 04
decisions: 0168, 0305, 0492

## Change
0492 point 5's image: beside every depth copy, the window's and each target's, one per frame
slot, a half-size image the clouds are marched into, shown through a texture slot of its own as
the depth copy is. Nothing draws into it yet. Read the headers of `render/src/target.c`,
`render/src/target_own.c` and `render/src/texture.c`; in `render/src/device_calls.h` the target.c,
target_own.c and texture.c entries; in `render/src/device_parts.h` `voe_render_target`,
`voe_render_texture_slot` and `voe_render_target_slot`.

- `render/src/device_parts.h`: `voe_render_target` gains `clouds`, an allocated image; the texture
  slot can say it shows a target's clouds image (as `depth` says the depth copy), with a
  window's value beside `VOE_RENDER_WINDOW_DEPTH`; `voe_render_target_slot` gains
  `clouds_texture` beside `depth_texture`. Comments: RGBA16F, half the target's size rounded up
  and at least 1 × 1, premultiplied light and transmittance, resting in SHADER_READ_ONLY_OPTIMAL
  outside the clouds' own march.
- `render/src/target.c`: the window's clouds images built, settled and torn down with its depth
  copies, at every build and resize, through `voe_render_target_image_build` (colour attachment
  and sampled usage); the window's clouds slot claimed at the first build as its depth copy's is.
  The header's paragraph on the depth copy names the clouds image.
- `render/src/target_own.c`: the same for a caller's targets, built, resized and freed with their
  depth copies, a third slot claimed at create and given back at destroy; the header's "a target
  costs two" becomes three.
- `render/src/texture.c`: `voe_render_texture_write_descriptors` names the frame slot's clouds
  image for a clouds slot, as it does the depth copy for a depth slot. Keep the file under 800
  lines; a short helper beside the write is enough.
- `render/src/device_calls.h`: any call whose entry changes says so.
- `render/src/src.md` (target.c, target_own.c, texture.c): follow, each at most 300 characters.

## Done when
The tests `render/targets`, `render/offscreen`, `render/depth_copy` and `render/textures` pass
after the folder's build.
