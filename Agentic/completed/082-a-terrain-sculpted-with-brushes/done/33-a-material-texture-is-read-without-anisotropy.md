# 33 — A material texture is read without anisotropy
folder: render/src
after: 32
decisions: 0168, 0369

## Change
`render/mips` fails: up close every pixel of the 4×4 checker reads 187, a
black-white blend, where NEAREST magnification must give hard texels.
Turning anisotropy off on the SMOOTH sampler makes all three cases of
`render/tests/mips.c` pass on llvmpipe (tried while planning; with it on the
test fails at every commit since 063 card 03). That is 0369's own clause: on
the card the checks run on anisotropy softens magnified texels, so it is off
everywhere.

File: `render/src/texture.c`. Where the three samplers are filled in, the
SMOOTH one gets `anisotropyEnable = VK_FALSE` and `maxAnisotropy = 1.0f`
like the other two, and stops reading `device->max_anisotropy`. The header
comment's "A MATERIAL TEXTURE HAS A MIP CHAIN" paragraph drops "at up to 8×
anisotropy where the card has it" and gains the point that anisotropy is off
because on the checked card it blends magnified texels (0369), so a low-angle
surface is plain trilinear.

Leave `render/src/device.c` (over 800 lines) and `device_internal.h` as they
are: the device still asks for `samplerAnisotropy` and keeps
`max_anisotropy`, now read by nobody; say so in one phrase in the same
header paragraph so a later card can remove both.

## Done when
- `grep -n "max_anisotropy" render/src/texture.c` prints nothing.
- `bash ~/.claude/skills/checks/scripts/checks.sh --folder render/src`
  prints FINDINGS: 0; it runs the test `render/mips`, which passes.
