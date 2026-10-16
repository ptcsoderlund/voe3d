# 0433 — A model's levels are called LODs on screen, and there are up to six
date: 2026-10-16
by: tech-lead

## Decision
For 089 bug 06. Everywhere the editor shows a model's levels to a person, it says **LOD**: the Assets menu row
is **Make LODs**, the question is "LODs of <file>" with a row **LODs**, and the Inspector's Level reads **LOD**
(LOD 0 is the full model). Only the words on screen change; the `.parts` keys, the levels files, the code's
names and the Frame panel stay as they are. The count is 1 to **6** LODs in all, LOD 0 included, and the
default is **6**, so a model with no count in its `.parts` gets six; the three-quarters rule (0425, 0430
point 3) still ends the list sooner when the simplifier cannot get well below the LOD before, so small models
get fewer. Levels files made with the old default are made again at their next load.

## Reasoning
The sponsor's words: "LOD" is what game makers call them, and "Levels" read as something else. Six rather
than four because each further LOD costs little (at half per step, LODs 4 and 5 add about 3 % to what the
model holds) and the three-quarters rule already stops a model at the LOD it can no longer cut. Above six
gains nothing: a model at LOD 5 is a few dozen pixels, where its draw costs more than its triangles.
Rejected: renaming the file keys too (every `.parts` already written would need reading both ways for no one
who sees them); keeping 4 as the default with 6 allowed (the sponsor asked for six).

## Replaces
Amends 0430 point 3 (1 to 6, default 6) and point 6 (the question's words and its LODs row), 0429 point 2 (the
menu row's name).
