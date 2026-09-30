# 0302 — A fold is an identity flag, and Add component brings what a type needs
date: 2026-09-30
by: planner

## Decision
For 043, how 0300 is built:
1. **A parent row needs no transform.** `voe_scene_parent` stops naming the transform as needed;
   `voe_scene_parent_set` takes a child or a parent without one. A bare child gets only the
   structural requests; a child with a transform under a bare parent gets its world place as its
   row, since the chain ends at a parent without a transform (as `parent_component.h` already says).
2. **The Scene list's fold is `folded`, a BOOL on `voe_scene_identity`**, default false, after
   `name`. It is saved in scene text, carried by undo and prefabs, and shown in the Inspector's
   identity section like any field. A file without it reads as unfolded. The Scene list toggles it
   through the identity's replace intent, one undo step.
3. **Add component brings the needed chain.** Adding a type that needs a type the entity lacks
   (`voe_ecs_component_needs`) also queues that type's default row, first, and so on up the chain
   (a body brings a collider fitted to any shape, which brings a transform).
4. **No Remove on a row another row of the entity needs.** The Inspector hides Remove for a type
   while any other row on the entity names it as needed; the transform leaves `dock.c`'s kept list.
5. **A project type may name what it needs**: `voe_game_project_type` gains `needs`, a key or
   NULL, registered with `voe_ecs_component_needs_set`. The tank game's placed types name the
   transform; Breakable does not.

## Reasoning
An identity flag rides every path a scene takes (save, undo, prefab, duplicate) with no new table,
and the Inspector already shows and edits a BOOL, so no section, no Remove and no editor-only type
the game would have to know. Rejected: a fold list in `project.voe3d` (not undone, and an open
prefab's ids are another file's); a separate `folded` component (a section with Remove in the
Inspector, and a table for one bit). Following `needs` keeps the editor naming no type: the
transform is brought because shape, light, camera, model, emitter and collider already say they
need it.

## Replaces
Nothing. Carries out 0300; amends 0221's kept list.
