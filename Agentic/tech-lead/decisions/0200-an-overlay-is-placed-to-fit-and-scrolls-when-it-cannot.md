# 0200 — An overlay is placed to fit, and scrolls when it cannot
date: 2026-09-20
by: tech-lead

## Decision
An overlay opened from a widget is placed where it fits, decided every frame it is open, not once when it
opened. It opens on its natural side of the widget — below, for a dropdown's list — when there is room there.
When there is not but there is room on the opposite side, it opens on that side, its near edge against the
widget. When it fits on neither side it opens on the side with more room, capped to that room, and scrolls
within itself: the wheel over an open overlay scrolls the overlay and not the panel behind it, the overlay
keeps its size and place while scrolling, and every item in it can be reached and chosen, the last one
included. Because the side is chosen each frame, an overlay opened below flips above when the panel scrolls
its widget toward the bottom edge, rather than being cut off. An overlay is never drawn cut off with items
that cannot be reached.

## Reasoning
A dropdown near the bottom of the Inspector opened downward into no room and its last kind could not be
picked at all — the bug found in 012. Flipping alone fixes every list as short as the three shape kinds, and
scrolling alone leaves you working a two-row slot near the panel's edge; taking both, in that order, keeps the
common case whole and still handles the long named lists later fields will have. The alternatives: always
scroll and never flip, one behaviour but a cramped one; always flip by which half the widget sits in, cheapest
but a list longer than the panel is still unreachable.

## Replaces
Nothing. Amends 0199, which fixed an overlay to its widget and clipped it to the panel; the fit is measured
against that same panel.
