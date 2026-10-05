# 0359 — A texture seen smaller than itself reads a mip chain, and keeps its hard texels up close
date: 2026-10-05
by: tech-lead

## Decision
Material textures (a model's colour, normal and emission maps) get a full mip chain. Seen smaller than their own
size, they are filtered across texels and between levels (trilinear), so distant surfaces stop sparkling and
crawling as the camera moves. Seen larger, they keep the hard-edged texels they have today: magnification stays
NEAREST. Anisotropic filtering is the planner's call when the card has it. The glyph atlas (0075, 0079) and sprite
sheets (ADR-0082) keep their own sampling unchanged. Textures stay uncompressed RGBA8 for now; block
compression is later work, waiting on texture budgets that need it.

## Reasoning
Every vendor guide says to mip: a minified texture without mips thrashes the texture cache and shimmers. The hard
texel look is only visible up close, where magnification decides it, so mipping minification costs no look.
0075 already chooses sampling per texture from a named set, so this means adding one more sampler to that set.
- Keep NEAREST with no mips: shimmer and cache misses at every distance, for nothing.
- Go LINEAR both ways: loses the chosen up-close look.
- BC7/BC1 compression now: needs an encoder in the cook step, and no game is near a texture budget yet.

## Replaces
Nothing. Amends the texture creation of 0075 and 0278.
