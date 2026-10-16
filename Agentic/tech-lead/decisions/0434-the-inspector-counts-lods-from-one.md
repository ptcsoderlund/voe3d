# 0434 — The Inspector counts LODs from one
date: 2026-10-16
by: planner

## Decision
For 089 bug 07. The Inspector's LOD rows count from one: the full model is **LOD 1**, the coarsest of six
**LOD 6 of 6**, and a model without LODs **LOD 1 of 1**. The row shows the level drawn plus one; the count
after "of" is unchanged. Only the Inspector's words change: the code's level indices, the `.parts` keys, the
levels files and a `.glb`'s `_LOD<n>` node names still count from zero.

## Reasoning
The sponsor's words: only programmers count from zero, and "LOD 1 of 6" to "LOD 6 of 6" reads as a count of
six. Moving the base in the code or the files would touch every reader and every file already written for a
change no one sees; the Blender `_LOD<n>` names follow the convention authors already use.

## Replaces
Amends 0433's "LOD 0 is the full model" on screen; the rest of 0433 holds.
