# 0337 — The authored room is 128, and a make with no identity room refuses
date: 2026-10-03
by: planner

## Decision
For 052 bug 02, carrying out 0335:
1. **The cause.** The tank game's level holds 32 identities (19 entities, a tank body's part and two
   houses' six parts each), which is `VOE_GAME_WORLD_AUTHORED`. Add entity queued the identity, the
   apply dropped it silently as the table was full (ecs/structure.h), and the entity lived on bare.
2. **`ecs` answers a type's room.** `voe_ecs_component_capacity(world, type)` is the capacity the
   type was registered with. With `voe_ecs_component_count` it says whether a row will fit.
3. **The authored room is 128.** `VOE_GAME_WORLD_AUTHORED` goes from 32 to 128 (room for four levels
   of today's, still half the drawn room). `VOE_EDITOR_SCENE_ROWS` is defined as that number, so the
   two cannot differ; the interface's node and element budgets are counted again for 128 Scene list
   rows and 129 dropdown rows. `VOE_EDITOR_UNDO_TEXT` goes from 32 KiB to 64 KiB, so a full level's
   text (about 300 bytes an entity) still fits one state.
4. **No make leaves an entity without an identity.** The editor's Add entity, model drop, prefab
   drop and Duplicate refuse, as "the scene is full", when the identity table holds its capacity,
   before any entity is created. `voe_authoring_prefab_read` refuses, creating nothing, when the
   identity table has no room for every entity it would make.
5. **A file read needs nothing new.** Every `[N]` a scene file holds is an identity by the format
   (authoring/scene_read.h), and undo reads the same text; a world that runs out of room on a read is
   discarded, as today.

## Reasoning
0335 says every editor entity has an identity always; the only way one went missing was a full
table, so the fix is a refusal at every make plus room for a real level. A capacity query is one
getter and lets `authoring` check room without knowing `game`'s numbers. Rejected: checking the
identity's add after the queue applies and destroying the entity (the editor has already selected
and counted it by then); the game's macro in `authoring` (an edge upward); 256 (the authored room
would equal the drawn room, leaving no draws for spawned things in a full level).

## Replaces
Nothing. Raises the 32 of 0283 point 11; that point's rule (spawned things carry no identity) stands.
