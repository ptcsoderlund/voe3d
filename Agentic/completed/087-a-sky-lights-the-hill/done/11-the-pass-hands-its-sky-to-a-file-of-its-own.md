# 11 — The pass hands its sky to a file of its own
folder: render
after: none
decisions: 0168

## Change
`render/src/pass.c` is 813 lines and the next cards change its sky parts. Move them out, behaviour
unchanged, byte for byte.

- `render/src/pass_sky.c` (new): `place_sky` and `place_sky_light` from `render/src/pass.c`, made
  public as `void voe_render_pass_place_sky(const voe_render_pass_camera *camera, struct
  voe_render_frame_block *block)` and `void voe_render_pass_place_sky_light(...)` (same parameters).
  Header comment: the "ITS SKY" paragraph of `pass.c`'s header, moved here; that `pass.c`'s
  `voe_render_pass_begin` calls both, in that order, after the block's lights.
- `render/src/pass.c`: the two functions and the `sky.h` include go (keep any include still used);
  the two calls become the public names; its header's "ITS SKY" paragraph shrinks to one line naming
  `pass_sky.c`.
- `render/src/device_calls.h`: a `pass_sky.c` group declaring the two, beside `pass.c`'s if it has one.
- `render/src/src.md`: the `pass.c` entry no longer claims the sky; one new entry for `pass_sky.c`, at
  most 300 characters.

## Done when
`wc -l < render/src/pass.c` prints under 800, and the tests `render/sky`, `render/sky_light`,
`render/exposure` and `render/haze` pass after the folder's build.
