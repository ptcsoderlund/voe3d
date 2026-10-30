# 0502 — A fold is named by section type, or by landscape and kind index
date: 2026-10-30
by: tech-lead

## Decision
For 108, a fold kept in the person's global `editor_settings` (0496 point 3) names its section without
an entity:
1. **An Inspector section** is named by what it shows: a described component by its component key, a
   hand-written section (Sculpt, Layers, Foliage, Brush, LOD) by its section word. Folding a section
   folds it on every entity that shows it, in every project.
2. **A foliage kind's box** is named by its landscape file's path and the kind's index in that
   landscape. Every kind folds on its own, even two kinds on the same model; dropping a new model on a
   kind keeps its box's fold. Adding a kind clears any fold kept for its index, so a new kind starts
   open (0496 point 2). Moving or renaming the landscape file forgets its kinds' folds.
3. **Only folded things are written.** A line exists for each folded section or kind; no line means
   open, so the file does not grow with every section ever shown.

## Reasoning
The settings file is one for every project and scene, and entity ids are per scene, so a fold cannot
name an entity. A section folded by type is what "the sections I folded are still folded" reads as
across scenes. A kind's index is the one name it has that never collides within its landscape (two
kinds may name the same model, see the cognition entry foliage-kind). Losing folds when a landscape
moves costs a click, never a wrong state. Alternatives: kinds named by model (two kinds on one model
fold together); kinds not remembered (test 7 holds for sections only).

## Replaces
Nothing; settles the question 0496 point 3 left open.
