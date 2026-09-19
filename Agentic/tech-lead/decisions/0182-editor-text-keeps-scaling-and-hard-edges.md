# 0182 — Editor text keeps scaling with the window and keeps hard edges; no stroke may vanish
date: 2026-09-19
by: tech-lead

## Decision
The editor's text keeps scaling with the window's height (ADR-0104) and keeps hard, un-anti-aliased
edges (ADR-0078). Where text lands at a size that does not line up with the screen's pixels, the fix
is the edge cutoff: the point at which a screen pixel counts as part of a letter is tuned so that
every stroke of every letter keeps at least one screen pixel. A stroke one screen pixel wider than
its neighbours is accepted, as bug 01 of 009 already says; a stroke or letter losing all its pixels is
a defect. This holds for every font the editor offers, Pixel Operator and Oxanium alike.

## Reasoning
The sponsor compared the editor with CLion and chose the smallest change that keeps the engine's
rules. Alternatives rejected: fixing the editor's text to the display scale so it no longer shrinks
with the window (changes ADR-0104 for the editor); anti-aliasing text edges (breaks ADR-0078 for
text). Either can be reopened if tuning the cutoff is not enough.

## Replaces
nothing
