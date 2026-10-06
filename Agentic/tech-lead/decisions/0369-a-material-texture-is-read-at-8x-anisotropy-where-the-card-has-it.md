# 0369 — A material texture is read at 8× anisotropy where the card has it
date: 2026-10-06
by: planner

## Decision
For 063: the sampler for material textures (`VOE_RENDER_SAMPLING_SMOOTH`, trilinear with NEAREST magnification
per 0359) has anisotropic filtering on at 8×, or the card's `maxSamplerAnisotropy` if lower. `render` asks for
the `samplerAnisotropy` device feature when the card offers it and leaves anisotropy off when it does not; a card
without it is not refused. The glyph, sheet and shadow samplers stay without it. If, on the card the checks run
on, anisotropy softens magnified texels (render's mip test fails with it on and passes with it off), it is off
everywhere and the card that found it says so in `render/src/texture.c`'s header.

## Reasoning
Trilinear alone picks its level by the longer footprint axis, so a surface seen at a low angle (063's test 3)
goes calm but turns to mush well before the far end. Anisotropy keeps it sharp along the shorter axis for little
cost on any desktop card; 8× is the usual point past which the gain is hard to see and the cost keeps rising.
- 16×: twice the worst-case taps for a difference few can see.
- Off: a blurred band where test 3 looks.
- Required feature: refuses cards for a nicety.

## Replaces
Nothing. Settles the anisotropy 0359 left to the planner.
