# 0309 — A VPL shines from half a cell behind its surface
date: 2026-10-01
by: planner

## Decision
For 046, amending 0308 point 4: in the gather, each VPL's distance and direction toward a probe are taken from
its position moved back along its unit normal by half `VOE_RENDER_BOUNCE_SPACING` (1 m). A probe on or just
behind a lit face therefore receives that face's light, so trilinear reads 1 m off a face no longer blend in an
unlit probe. The flux, the cosine max(0, n·−ω), the 1 m² floor and the 1 mm skip are unchanged, measured from
the moved position.

## Reasoning
Probe centres sit at odd world metres; a face on such a plane (the tank game's 2 m boxes at whole metres) put a
probe in its plane that saw nothing of it, halving the tint beside it (card 16's block: 0.031 where 0.05 is
asked; 0.08 with the wall moved off the plane). Moving the source is one line in the gather and costs nothing.
- Probe relocation: per-probe state and a search, a feature of its own.
- Visibility-weighted interpolation: needs a visibility term 0307 rules out.
- A 1 m grid: an eighth of the volume for the same probe count.
- Moving the test's wall: hides a fault the shipped scene has.

## Replaces
Nothing. Amends 0308 point 4.
