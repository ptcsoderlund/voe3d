# 0276 — The fill fades out over the first third of the sun's reach
date: 2026-09-27
by: planner

## Decision
For 0275's fade, the sun's reach on a surface is `reach = saturate(dot(N, L)) × shadow`
(L toward the sun, shadow the cascade lookup, 1 with no shadow maps). The fill's weight is
`1 − smoothstep(0, 0.3, reach)`: whole where the sun does not reach, none from a reach of 0.3
up. The fill term becomes `weight × fill × base colour`, still outside the shadow factor
otherwise. Zero fill and `unshaded` are as in 0273.

## Reasoning
A linear `1 − reach` would still tint a floor the sun meets at a slant (a reach of 0.7 keeps
30 % of the fill), which is the bug the human reported. Cutting the fill off at a small reach
keeps every surface the sun really lights untouched, and the smoothstep keeps a shadow's soft
edge and the terminator free of a seam. Rejected: a hard cut (a visible line), linear (tints
slanted sunlit surfaces). 0.3 is a constant in the shader, not a field.

## Replaces
Nothing; fills in the curve 0275 leaves open.
