# 02 — dev's picture shrink stops claiming the engine has no mips
folder: dev
after: 01
decisions: 0168, 0359

## Change
Comments and map only; no behaviour changes. The halving in `dev` stays: it still saves tens of megabytes of
arena, upload and GPU memory for the two oversized pictures. What is now false is its reason.

- `dev/src/shrink.h`: the header's "IT IS NOT MIPMAPPING" and "WHY BOTHER" parts say the engine has no
  mipmaps and that shimmer remains. Rewrite them: material textures now get a mip chain (0359), so the
  shimmer is the sampler's job; this file only picks a smaller top level to save memory and upload time. Keep
  the numbers on memory; drop the claims about the caption smearing and aliasing remaining. The comment on
  `VOE_DEV_SHRINK_LONG_SIDE` stays unless it repeats the old claim.
- `dev/src/cubes.c`: the comment above the `voe_dev_image_shrink` call ("this engine samples one level with
  no mip chain") says the same new reason in a line or two.
- `dev/src/src.md`: the `shrink.h` entry no longer says "why this is not mipmapping and cannot be"; it says
  the header gives why the top level is shrunk though the engine mips.

## Done when
`grep -rniE "no mip|has no mipmaps|cannot be" dev/src/shrink.h dev/src/cubes.c dev/src/src.md` prints
nothing.
