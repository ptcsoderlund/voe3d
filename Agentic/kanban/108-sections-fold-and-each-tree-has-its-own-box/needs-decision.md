# 108 needs a decision: what names a fold in `editor_settings`

## Question
0496 point 3 keeps what is folded in the person's global `editor_settings`, beside the `dock_*`
lines (0491), not in the project. That file is one for every project and scene, so a fold line
must name its section without an entity id (ids are per scene). What does a fold line name, and so
what folds together?

- An Inspector section: the described component sections (one shared path, `component_panel` in
  `editor/src/inspector.c`) and the hand-written ones (Sculpt, Layers, Foliage, Brush, LOD).
- A foliage kind's box: a kind is `kind<k>="Assets/<model>.glb"` in its `.landscape`, index k.
  Nothing stops two kinds of one landscape naming the same model, and a drop on a kind's row
  replaces its model in place.

Precog and the scout found no decision on it; the behaviour shows on screen (folding a light's
section on one light either folds it on every light or does not), so it is not the planner's.

## Options
1. **Sections by what they show; kinds by landscape and model.** `fold section <component key or
   section word>` and `fold kind <landscape path> <model path>`, a line per folded thing, absent
   meaning open. Folding Light folds it on every light in every project. Two kinds of one
   landscape on the same model fold together; a dropped replacement model starts open.
2. **Sections by what they show; kinds by landscape and index.** `fold section …` as in 1, and
   `fold kind <landscape path> <k>`. Every kind folds on its own, and a replaced model keeps its
   box's fold. Adding a kind clears the line for its index, so a new kind starts open (0496 pt 2)
   even if an earlier kind at that index was folded. Moving or renaming the landscape file forgets
   its kinds' folds.
3. **Sections remembered, kinds not.** Sections as in 1. Kinds' folds last only while the editor
   runs. That is simplest, but test 7 then holds for sections only, which the feature does not say.

## Recommendation
Option 2. A section folded by type is what "the sections I folded are still folded" reads as across
scenes, and the index is the one name a kind has that never collides within its landscape. Losing
folds when a landscape is moved costs a click, never a wrong state. Lines are written only for
folded things, so the file does not grow with every section ever shown.
