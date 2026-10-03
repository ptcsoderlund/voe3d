# 0315 — The shadow side may take a faint trace of the bounce
date: 2026-10-01
by: tech-lead

## Decision
For 046 bug 03, answering the needs-decision after card 29 blocked. 0312's shadow-side bound is loosened:
in the tank project's `main.scene` capture, the ground in a box's sun shadow at the foot of its shadowed
face reads within 8/255 in every channel of the same pixels drawn with the bounce off, at sun 1 and at π;
render's SHADOW SIDE test holds within 4/255 in every channel. Everything else stands: 0312's rule that a
surface bounces only out of its lit side (no VPL moved behind its face), 0310's tint (blue at 1 m at least
blue at 8 m + 12, with 4 m between them, at sun 1 and π) and its proof on the hardware card through the
editor's scene-view path with the scene loaded from the file, 0311's single gain, 0314's allowance o chosen
as the smallest that works with the lowest whole gain, and the rule that the proving test fails on the
leaking build (it does: before 0313 the shadow side read 173 against 60). Card 29 measured o 0.25 at gain
10 passing these bounds; the coder confirms it and takes it. The sponsor judges the look at 046's test; if
the remaining trace shows, finer probes near the eye become a work order of their own, not 046's.

## Reasoning
A 2 m probe grid without ray tracing or distance fields (0307) carries about one cell of a lit face's light
past it, so 2/255 could not be met together with a visible tint. Card 29 showed the leak is now a faint blue
in a dark shadow (47,45,31 against 47,45,24 at sun π), not the washed-out shadow the sponsor reported.
- Relax 0310's fade instead: still misses the shadow bound, and the tint would start away from the wall.
- A second, 1 m probe grid around the eye: halves the leak honestly, but costs GPU each frame and is a new
  work order that holds 046 open.

## Replaces
Nothing. Amends 0312's 2/255 bound and 0314's measure.
