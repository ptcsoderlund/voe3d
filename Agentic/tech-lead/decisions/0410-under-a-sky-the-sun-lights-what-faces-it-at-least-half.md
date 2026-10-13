# 0410 — Under a sky the sun lights what faces it at least half
date: 2026-10-13
by: planner

## Decision
For 087 bug 04.

1. Where the sky is `drawn`, light 0's reach on the lit path is `smoothstep(0, 0.05, N·L) ×
   lerp(0.5, 1, saturate(N·L)) × shadow`, not `N·L × shadow`: a surface the sun reaches at more
   than about 3° reads at least half lit, a low sun on flat ground included. The two constants are
   named in `render/shaders/lighting.slangh`.
2. That one reach feeds everything light 0's reach fed: its radiance (diffuse and specular) and its
   fill weight (0276, 0408), so with the 0.3 fade a surface the sun reaches past about 3° gets none
   of the fill or the sky's diffuse, whatever `fill` is.
3. Lights 1 and up, the water path, and every world without a drawn sky keep `N·L × shadow`, so the
   tank and coin games are unchanged byte for byte.

## Reasoning
The human, with a low sun on the hill: the sun "hardly lights anything", and raising fill brightened
the sunlit ground. Both come from N·L: a sun 5° up gives flat ground a reach of 0.09, a tenth of full
light, and under the fade's 0.3, so the fill counted it as shade. 0406 asks for games that look like
games: a lifted cosine with a floor, against a plain wrap (half-Lambert): wrap lights the faces turned
away and leaves nothing for the fill to do there. Against lowering the fade's reach alone: the ground
would stop taking fill but stay near black. Gated by the sky because only 087's worlds have one, and
0405 keeps every older world's picture.

## Replaces
Amends 0408 point 1: light 0's reach under a drawn sky is the lifted reach of point 1 above.
