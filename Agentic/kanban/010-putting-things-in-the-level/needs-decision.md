# Needs decision — how the editor adds and removes components and entities

## The question
Feature 010 has the editor change which rows exist in the project world, not only their values:
add a component to an entity that already exists (Add component, at the type's default values),
remove one (Remove), and destroy an entity (Delete). Duplicate and the Add menu create entities,
which rule 3's creation exception and ADR-0125 point 4 already cover.

Nothing decided allows the other three. Rule 3 (0168): a component is written only by its own
system, and the only exceptions are a folder's typed creation call and `authoring`'s reader
adding rows to entities they have just made. ADR-0152 point 2 says that exception is "exactly
this and no wider" and "never removes a row". Rule 4: change another module's data by an intent.
ADR-0134 gave editing a value one generic path (the replace intent); there is no generic path
for adding a row at its defaults, removing a row, or destroying an entity.

It also needs a component's default row, generically. The Inspector is driven by descriptions
alone (ADR-0134, `editor/src/inspector.h`), and a zeroed row is not a valid default for every
type: a zeroed transform has scale 0 and a zero quaternion. Nothing registers a default today.

This reaches past 010: it amends a standing rule of 0168, grows `ecs`'s public surface that
every component folder registers against, and is the question game logic's spawn and despawn
(milestone 3, 0186) will ask again.

## Options
**A. The world owns which rows exist; a system owns their values.** `ecs` gains a structural
queue (add a row with given bytes, remove a row, destroy an entity), applied at one point in
the frame before the systems run. A described type registers its default row beside its
replace intent (`voe_ecs_component_default_set`, as ADR-0134 did for the intent). Rule 3 is
amended for everyone: structure through the world's queue, values through the owner's intent.
Serves the editor now and game spawn/despawn later; the most machinery.

**B. A narrow exception for the editor, in the shape of ADR-0152.** The editor may add a row
at its type's registered default, remove a row, and destroy an entity in the project world it
owns, directly and between frames, and never calls `voe_ecs_component_set`. Defaults are
registered in `ecs` as in A. The drains already tolerate an entity that is gone
(`voe_ecs_component_set` returns false). Consequence: rows another system derived from a
removed one (a shape's runtime-only mesh and material) are cleaned up by that system, so
`3d`'s shape system learns to drop the mesh and material of an entity whose shape is gone.
Least machinery; game logic's spawn and despawn is decided when it arrives.

**C. Every owning folder takes the change through its own system.** `ecs` stores a remove
intent per type beside the replace intent; each owning system drains it, and each folder
exposes an add-at-default. Fits rules 3 and 4 as they stand, but grows every component
folder and every drain for a need only the editor has today.

## Recommendation
**B.** It is the smallest change that lets 010 be built, it mirrors an exception the tree
already accepted (ADR-0152), and it decides nothing for game logic before milestone 3 asks.
Whatever is chosen, the default row belongs in `ecs` beside the replace intent, set by the
declaring folder.
