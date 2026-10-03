# 0311 — The bounce's gain is one constant where lit surfaces read it
date: 2026-10-01
by: planner

## Decision
For 046, carrying out 0310's raised strength: the probe grid keeps the irradiance the gather measures,
unchanged; `render/shaders/lighting.slangh` multiplies the bounce it reads, E/π, by one constant,
`VOE_BOUNCE_GAIN`, before the fill floor's max. Its value is the lowest whole number at which the tank
project's `main.scene`, captured through the editor's scene view on a hardware card, meets 0310's
12/255 at sun 1 and at π. Tests that hold a physical bounce value say the gain; tests that compare
near with far keep their claims.

## Reasoning
One number in the one place a lit surface reads the bounce is the cheapest cheat 0307 allows: no
schedule, update or grid image changes, every pass that reads the grid gets the same look, and a later
tone map can lower it in one line. Scaling the VPL flux instead would hide the gain inside the update,
where the render tests measure irradiance.

## Replaces
Nothing. Amends 0308 point 4, as 0310 does.
