# 108 — Sections fold, and each tree has its own box

## What
In the Inspector, every section can be folded away to its title line and opened again. Titles are
drawn in a heading style that stands out from the rows below them. When I select the landscape, each
foliage kind in the Foliage section is its own box, titled with its model's name, that folds and opens
on its own. Clicking the name still picks the kind to paint with. The editor remembers what I folded,
even after a restart (0496).

## Why
With several trees in the Foliage section, every kind's settings are one long run of rows and I lose
track of which model I am looking at. Folding belongs in the interface for every panel, not just this
one.

## How to test
1. Open the hill and select the landscape. In the Foliage section, each kind is a separate box with its
   model's name as a heading, clearly apart from the kind above and below it.
2. Click the arrow on a kind: its settings fold away and only its title line stays. Click it again:
   they come back as they were.
3. Click a kind's name: it is picked for painting and lit, as before, and it does not fold. Paint a
   little: the picked kind grows.
4. Fold all kinds but one: the section is short enough to see the open kind's settings without
   scrolling. Change a setting in it: the forest follows as before.
5. Drop a new model to add a kind: its box starts open.
6. Select a light, a model and the clouds row: each Inspector section has a heading and folds and
   opens the same way.
7. Close and reopen the editor: the sections I folded are still folded.
8. Undo still steps through my setting changes, not through folding.
