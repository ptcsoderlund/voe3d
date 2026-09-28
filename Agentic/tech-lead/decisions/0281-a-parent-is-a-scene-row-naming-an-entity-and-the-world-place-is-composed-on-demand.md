# 0281 — A parent is a scene row naming an entity, and the world place is composed on demand
date: 2026-09-28
by: planner

## Decision
For 037, filling in what 0271 leaves to the planner:

1. **The parent is its own component, `voe_scene_parent`, in `scene`**: one ENTITY field `parent`.
   No parent row is a root. It needs a transform, has a default row (no entity), no menu path and
   no replace intent, so Add component never offers it and the Inspector shows it without editing
   it. Scene text, undo and the cook already carry ENTITY fields as authored ids, so a saved,
   undone or played tree needs nothing new from `authoring`.
2. **The transform row stays what it was, now relative to the parent.** Nothing is cached.
   `voe_scene_transform_world(world, entity)` composes the rows up the chain each time it is
   asked; `voe_scene_transform_between` blends each link by the lag and then composes, so it hands
   back a world transform too. Every reader that places, draws, picks or collides reads one of the
   two; the Inspector alone reads the row.
3. **Composition is position, rotation and scale, not a matrix.** World position = parent position
   + parent rotation · (parent scale ∘ child position), in double; rotation = parent · child; scale
   = parent scale ∘ child scale. Exact while scales are uniform; a rotated child of a
   non-uniformly scaled parent shows no shear. `voe_scene_transform_relative` is its inverse; a
   zero parent scale component gives a zero relative component, not a division by zero.
4. **Parenting is a structural change plus one transform intent.** `voe_scene_parent_set(world,
   child, parent)` queues the removal of the child's parent row, then (for a live parent) an add
   naming it, then a transform intent whose row keeps the child's world place under the new
   parent. A zeroed parent unparents. A parent that is the child or under it asserts; callers
   ask `voe_scene_parent_within` first. The editor's drag, the Inspector's Remove and game code
   all go through it.
5. **Deleting a parent deletes its tree.** `voe_scene_parent_tree` lists an entity and everything
   under it; the editor destroys each through the structural queue, and refuses when the scene's
   camera is among them (0218).
6. **Every walk is capped at `VOE_SCENE_PARENT_DEPTH_MAX` (32) links.** A loop can only come from a
   hand-edited file; it ends the walk there instead of hanging. A dead parent, or one with no
   transform, ends the chain as if there were none.
7. **The Scene list is depth-first**, each row indented by its depth. A row released on another
   row parents onto it; released on the list's "Scene" heading it becomes a root. Onto itself or
   something under it does nothing.

## Reasoning
A separate row keeps every scene saved before 037 reading with no warning, which a `parent` field
on the transform would not. Composing on demand is what the transform's header already argues for
the matrix: no second copy to invalidate. Position, rotation and scale are what every reader takes
(physics, gizmo, camera, sun); the tank's parts are unscaled. A structural change reuses the
queue undo and game code already use, so no new drain or intent type is needed. Rejected: a
cached world matrix column (a stale-cache problem for no measured cost); matrix composition (every
reader would need a matrix it does not take); a replace intent on the parent (the Inspector would
parent without keeping the world place).

## Replaces
nothing. Reads 0271, 0190, 0204, 0254.
