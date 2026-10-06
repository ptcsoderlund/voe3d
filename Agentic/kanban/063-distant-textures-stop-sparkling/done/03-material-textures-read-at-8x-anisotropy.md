# 03 — Material textures are read at 8× anisotropy where the card has it
folder: render
after: 01, 02
decisions: 0168, 0359, 0369

## Change
Carry out 0369: the SMOOTH sampler gets anisotropy when the card offers it.

- `render/src/device.c`: where the device's features are asked for (beside the
  `shaderSampledImageArrayNonUniformIndexing` request, which already asks only when available), request
  `samplerAnisotropy` in the core features when the card has it, and record on the device the anisotropy the
  samplers may use: min(8, `limits.maxSamplerAnisotropy`) when on, 1 when not. A card without it is not
  refused.
- `render/src/device_internal.h`: that field on the device struct, with a comment naming 0369.
- `render/src/texture.c`: the SMOOTH sampler sets `anisotropyEnable` and `maxAnisotropy` from that field;
  SHARP and FIELD keep it off. Replace the "Anisotropy is off in all three" comment with why SMOOTH has it
  (low-angle surfaces stay sharp along the short axis) and why the others do not (sheets and fields are
  seen face-on and one level).
- `render/tests/mips.c`: add a check: a 256×256 one-texel black/white checker on a large plane seen at a low
  angle (camera a little above it, looking along it); every pixel of the plane's far half is between 64 and
  220 on each channel. The up-close check must still pass with anisotropy on. If it fails only because of
  anisotropy, 0369 says turn it off everywhere: do that, keep the field at 1, and say so in `texture.c`'s
  header.
- `render/tests/tests.md`: the `mips.c` entry names the low-angle check.

## Done when
`ctest --test-dir build/debug -R "^render/(mips|best_practices)$"` exits 0, and
`grep -n samplerAnisotropy render/src/device.c` finds the request.

Human, after the build (feature.md `## How to test`): open the tank game in the editor; close up a textured
model's texels are hard squares; backed away and moving side to side it does not sparkle or crawl; along a
large textured surface at a low angle the far end is calm; editor text, world text and a sprite look as
before; press Play and check the same in the game window.
