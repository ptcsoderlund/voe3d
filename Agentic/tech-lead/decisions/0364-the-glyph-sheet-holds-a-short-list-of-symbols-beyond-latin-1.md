# 0364 — The glyph sheet holds a short list of symbols beyond Latin-1
date: 2026-10-05
by: planner

## Decision
For 059 bug 01.

1. `text`'s sheet keeps its one run, U+0020 to U+00FF, and adds a short fixed list of characters beyond it,
   each Oxanium carries, each with a slot of its own after the run and before the missing-glyph box. The list
   is one array in `text/src/font.c`, in codepoint order; a character on it is found by a search of that list.
2. The list starts with √ (U+221A), the Panels menu's tick (0363 point 3). A later character the editor needs
   is one more entry and the card that needs it adds it; a whole script is still its own decision (0247).

## Reasoning
0363 chose √ because Oxanium carries it, but the sheet only ever held Latin-1, so the tick drew as the box.
A list keeps the sheet small (it fits the 512-texel sheet with room) and the run's subtraction unchanged for
every ordinary letter. Rejected: widening the run to U+221A, eight thousand slots for one glyph; a Latin-1
stand-in such as `×` or `·`, which reads as close or as a bullet, not as open.

## Replaces
nothing. Amends 0363 point 3: the √ is drawn because the sheet now holds it.
