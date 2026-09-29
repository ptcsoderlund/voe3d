# 0283 — A prefab is a scene text, a placed copy saves its root, and game code spawns cooked prefabs
date: 2026-09-28
by: planner

## Decision
For 038, filling in what 0268 milestone 5 and 0270 leave to the planner:

1. **A prefab is a `.prefab` file under `Assets/`, in scene text (0149, 0150)**, holding one
   tree: exactly one entity with no parent (its root), ids local to the file, and no camera,
   light, prefab or part row. `authoring` writes one from a tree in a world: the tree's
   entities with their ids, the root's parent row left out, an ENTITY naming outside the tree
   written `0`, no kept sections. The prefab root's transform is where it sits while the prefab
   is open, and nothing else: no copy and no spawn takes it.
2. **Two rows in `scene`.** `voe_scene_prefab`, one 128-byte CHAR `path` (project-relative,
   `/`), on a placed copy's root: authored, needs a transform, no replace, no menu.
   `voe_scene_prefab_part`, runtime-only, one entity `instance`, on every entity made from a
   prefab, the root naming itself. Both are in `game/world.h`'s list.
3. **A placed copy is saved as its root alone**: identity, transform, parent and prefab rows.
   The scene writer skips an entity whose part row names another and writes only those four for
   one whose part row names itself. The cook is unchanged: it cooks the world as the editor
   holds it, copies expanded, so a played level needs no expansion.
4. **The editor expands.** After every scene text read into a world, and once a frame after
   the world step, every entity with a prefab row and no part row is read from its file: it
   gains the prefab root's rows it lacks (never identity, transform or parent); every other
   prefab entity is made with an identity whose id is above every id in the world, roots taken
   in ascending id; every one gets a part row. It is a load, rule 3's creation exception. A file
   that will not read or is refused says `Could not read <path>` in the notice, and the root
   still gets its part row, so it is not tried again each frame.
5. **Parts are shown, not edited.** The Scene list lists them under their root and marks the
   root with its prefab's file name. A part is selected by the list and by a pick; the
   Inspector shows its fields without controls and says it is part of the prefab. A part has no
   gizmo, no Delete, Duplicate, Add component or Remove, is never dragged or dropped onto in the
   Scene list, and takes no model drop. Deleting a root deletes its tree; duplicating one makes a
   new copy of its four rows, expanded the next frame.
6. **Making one**: a Scene list drag released over the Assets panel writes
   `<shown folder>/<root's name>.prefab`, then gives the root a prefab row and it and its tree
   part rows: the original becomes a placed copy. One undo step, unsaved. Refused, with a
   notice, when the project is untitled, a prefab is open, the tree holds a camera, light,
   prefab or part row, or the file exists.
7. **Placing one**: a `.prefab` row released over a view makes a root named after the file less
   `.prefab` (entities.h's name rules), its transform at 0277 point 8's point, no turn, scale
   one, and its prefab row. One undo step, unsaved; expanded the next frame.
8. **Opening one**: a prefab row clicked (pressed and released on the row) opens it. The level's
   scene text, unsaved edits and kept sections included, and its unsaved flag are set aside;
   the world is the prefab's text; the top bar names the prefab and shows Back. Save writes the
   prefab, refusing a world without exactly one root or with a camera, light or prefab row.
   Back is refused once while the prefab is unsaved, then reads the level's text back, so every
   copy is expanded from the saved file, with its unsaved flag. The level keeps its undo line
   across the visit; the prefab has its own, empty on each open. Play and Ship are refused while
   a prefab is open; New, Open and Close refuse once if the prefab or the level is unsaved. No
   prefab is opened, made or placed while one is open.
9. **The game gets cooked prefabs.** Play and Ship cook every `.prefab` under `Assets/` into the
   game tree's `prefabs.c`: per prefab, a function that queues its rows through the structural
   queue onto entities the spawn made, root first then ascending id, identity left out (spawned
   things are not authored), ENTITY fields mapped within the prefab, the root's transform the
   given position and rotation at scale one; and one table, `voe_game_prefabs_cooked`, naming each
   by its path under `Assets/` less `.prefab`. At most `VOE_GAME_PREFAB_ENTITIES` (32) entities.
10. **Spawning is the project seam's.** `voe_game_project_spawn(step, name, position, rotation,
    &root)` and `voe_game_project_remove(step, entity)` (the entity and its tree, queued) are in
    `game/project.h`, defined in `game/src/project.c`, and find the table in `step->prefabs`
    (NULL in the editor and in tests). An unknown name is a stderr line and false; a full world
    or queue is false with nothing left behind. What they queue lands at the step's structural
    apply. No pool: the world reuses slots.
11. **Room**: a game world holds 4096 entities, 1024 transforms and parents, 256 drawn things and
    a structural queue of 2048 requests and 256 KiB. Identities stay at 32.

## Reasoning
Scene text for a prefab reuses the writer, reader, Inspector and undo whole; a copy saved as its
root means a saved prefab reaches every copy at the next read with no second file to rewrite.
Parts carrying identities keep the Scene list, picking and undo's re-find unchanged; marking them
runtime-only keeps them out of every file. Cooking the expanded world keeps the game free of a
second expansion. The spawn sits in `project.c` because a project library is loaded with
RTLD_NOW against the editor, which does not link `game` whole-archive; `project.c` is already in
it, and the cooked table reaches it as data in the step, so the editor links no cooked symbol.
Rejected: overrides per copy (the feature keeps only the place); the whole tree saved in the
level with a link (every save of a prefab would rewrite scenes); nested prefabs (not needed by
the tank game); a pool of spawned things (slots are already reused and no cost is measured).

## Replaces
nothing. Fills in 0268 milestone 5 and 0270's prefabs; reads 0277, 0281, 0204, 0188.
