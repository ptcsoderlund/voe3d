# 0446 — Blender's texCoord −1 is no texture, and a 16-bit PNG is read at 8 bits
date: 2026-10-18
by: planner

## Decision
For 090 bug 01.

1. A texture reference whose `texCoord` is negative (Blender writes −1 for a picture on no UV map)
   reads as that slot having no texture: the material keeps its factors, nothing is reported.
   Another set than the first stays refused, in 0443's words.
2. A PNG of bit depth 16, in any colour type PNG allows at 16, decodes to RGBA8, each sample made
   8 bits by rounding, `(v + 128) / 257`. Depths below 8 stay refused, in 0443's words ("save it as
   8-bit"), until a file shows one.

## Reasoning
Both are in a valid file Blender writes for a downloaded tree. Point 1: the part the reference is
for has no UV map, so the picture could only be sampled at one texel; its factors are the closest
honest drawing. Point 2: `image.h` makes every picture RGBA8, and 16-bit detail past 8 is lost on
the card's RGBA8 format anyway; rounding rather than the high byte keeps white white and is one
division. Alternatives: refuse (0280 says no refusing), sample at UV 0 (one texel's colour over the
whole part), keep 16 bits (a second upload format through `render`).

## Replaces
nothing
