# 0199 — An overlay belongs to the widget it opened from and swallows the cursor
date: 2026-09-20
by: tech-lead

## Decision
An overlay a widget opens — a dropdown's list, and any menu, popup or tooltip after it — is positioned from
that widget every frame it is open, not once when it opened. It sits where the widget is now, so it moves with
the widget when the panel scrolls or the panel is resized, and it is clipped by the same panel: when the widget
scrolls out of view the overlay goes with it rather than floating over the rest of the editor. An overlay is
also solid over its whole outline: it fills its area opaquely, and every pixel inside that outline — the gaps
between its items, the padding at its edges, its border — is the overlay's for hovering and for clicking.
Nothing drawn under an open overlay hovers, highlights or takes a click through it. An overlay closing is the
only way the cursor reaches what was beneath.

## Reasoning
An overlay that remembers where it opened is wrong the moment anything moves, and an overlay that is only its
items lets the cursor fall between them into the widgets it is covering — both of the bugs found in 012. The
alternatives: close the overlay on any scroll, which is simple but loses your place on a stray wheel nudge;
let the overlay float unclipped over the editor, which leaves a list hanging off a button that is no longer
there. Making position and input follow from the widget each frame costs no more than either and is the rule
every later overlay can be built to — 013's scene view and 015's gizmo will both open things over a panel.

## Replaces
Nothing. Amends 0198, which left the open list's placement and input to the editor's panel.
