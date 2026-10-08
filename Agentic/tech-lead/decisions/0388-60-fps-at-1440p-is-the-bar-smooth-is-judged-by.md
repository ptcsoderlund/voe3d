# 0388 — 60 fps at 1440p is the bar smooth is judged by
date: 2026-10-08
by: tech-lead

## Decision
"Smooth" (0386) and "runs well" (0318) mean a steady 60 frames a second at 2560×1440 in the editor and in a
game, on the sponsor's own machines as 0318 has them judged. A feature that drops a frame below that while the
program runs is a bug against it, argued from the frame breakdown (0358). Tests still do not time anything:
the bar is the sponsor's, met by use. A card that adds steady per-frame work states which part of the frame
it adds to, so the breakdown can name it.

## Reasoning
The sponsor's call (2026-10-08), restated for 0387's bounce grids: the 60 fps at 1440p of 0368 was dropped with
that release and is now a standing goal for every feature.
- Leave "smooth" unnumbered: every feature argues its own idea of it.
- Put frame times in tests: 0318 already rejected numbers no machine in CI can hold.

## Replaces
Nothing. Sharpens 0318 and 0386 with a number; restores 0368's frame-rate bar as a standing rule.
