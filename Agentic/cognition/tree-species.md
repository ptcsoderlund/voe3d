# Tree species

What a `.tree` species file means, how it grows into variations, and how the store, the forest, the editor and the game hold and regrow them.

## Its settings
- `angle` is in degrees, as a landscape look's turn is. [assumed · planner 117 · 2026-11-20] see 0515
- `branching` counts branches per metre of the part of the parent above `crown`; twigs apply `crown` to their own branch's length. [assumed · coder 117-02 · 2026-11-20] see 0515
- `droop` is how far branches bend down, a negative value bending them up; `crooked` is how far branches wander from straight; both unitless. 0515 gave them no meaning, so the grower holds to these or both change. [assumed · coder 117-01 · 2026-11-20]
- `variations` must be a whole number (`2.5` is malformed); a whole number outside 1–16 is clamped by the lenient read and refused by the strict one. [assumed · coder 117-01 · 2026-11-20] see 0515
- A file with no `[Tree]` section is malformed even to the lenient read, as a material's is; only missing keys take defaults. [assumed · coder 117-01 · 2026-11-20]
- A species with an empty `bark` or `leaf` wears none there. [observed · coder 117-17 · 2026-11-24]

## The growth
- Where a branch meets its parent, a collar ring sits where the branch's axis leaves the parent's surface, its radius halfway between the branch's base radius and the parent's surface radius; taken literally, "widened to the parent's surface radius" would skirt every joint as wide as the parent. [assumed · coder 117-02 · 2026-11-20]
- Bark runs around a limb over whole metres of its base circumference (at least one), so bark texels keep about one size from trunk to twig and the seam tiles. [assumed · coder 117-02 · 2026-11-20]
- A grown model's own two materials are white placeholders named `bark` and `leaf`; what it wears is the species' own `.material` paths. [assumed · coder 117-02 · 2026-11-20] see 0515
- The size caps cut breadth first: a tree at every key's maximum (60 m, 20 branches a metre) reaches the cap with trunk and branches alone, no twigs or leaves. [assumed · coder 117-02 · 2026-11-20]

## Its levels
- A grown model's measures are kept as an own levels file's are, so the level summary and the cook take it as any model with its own LOD files. [observed · coder 117-03 · 2026-11-20]
- A grown model's levels all swap at 1, as default parts give, since it has no parts file; its swap distances rise level to level while its screen-size bounds fall. [assumed · coder 117-03 · 2026-11-20] see 0515

## In the store
- A species is a store entry with variations, kept loaded, partless and at stamp 0, so finding, counting and clearing see it as any entry; it takes one entry plus one per variation. [assumed · coder 117-04 · 2026-11-20] see 0515
- So the store's 128 entries, not the editor's 32-species table, limit how many species a project holds; the cap of 16 variations was chosen to fit them. [observed · coder 117-08 · 2026-11-20]
- A species holds its variations by their place in the store's table, which stays until the store is cleared, emptying the species too; a rename needs nothing more for them to follow. [assumed · coder 117-04 · 2026-11-20]
- Setting a species holds the `<path>#<v>` entries already in the store; a variation put after is not held until the species is set again, and a held one is checked loaded when looked up. [assumed · coder 117-04 · 2026-11-20]
- A rename moves `from#<v>` on every rename path, so a layer's or kind's material path of that shape would move too; none exists today. [assumed · coder 117-04 · 2026-11-20]
- Setting parts with no row empties a part's `wears`, so grown entries must be exempt or lose their bark and leaf. [observed · planner 117 · 2026-11-20]
- The game's watch keeps a loaded entry whose file is gone as failed at stamp 0, so an entry with no file behind it must be skipped by the watch. [observed · planner 117 · 2026-11-20]
- A store too full to set a species fails that species' grow as refused; the store has already said so once. [assumed · coder 117-06 · 2026-11-20]

## In the forest
- A copy's variation is stable across save and reopen: a foliage copy keeps x and z as whole millimetres, which the pick hashes. [observed · planner 117 · 2026-11-20] see 0515
- A species passes foliage's kind filter as is, so a kind is still looked up once; a copy whose variation is not loaded yet is neither drawn nor counted, as a kind not loaded. [observed · coder 117-05 · 2026-11-20]
- A row draws per drawn entry (kind and variation) within room the size of the old per-kind tables, sixteen whole `.glb`s; a copy past that room is left out and the left-out message names both causes. [assumed · coder 117-05 · 2026-11-20]

## Regrowing in the editor
- Under a splash, trees grow only when a species is marked: a project's first update marks them all, and a splash over the same unchanged folder grows nothing, so an import's or a load's splash does not regrow every tree. [assumed · coder 117-08 · 2026-11-20]
- A row is marked when it is new, when its values differ from the last read, or when it was still marked; the last keeps a pending regrow when Assets re-reads the tables. [assumed · coder 117-08 · 2026-11-20]
- A rename regrows the species under its new path, after the store has moved its entries there. [assumed · coder 117-08 · 2026-11-20]
- Whenever the materials table is read again, the trees table is read with it and the open species refreshed. [observed · coder 117-15 · 2026-11-24]
- A busy grow is ended when the folder changes and before a whole grow; ending it waits out its burst. [assumed · coder 117-08 · 2026-11-20]
- A stop still finishes the whole burst, all rows' variations being on one, so it saves the uploads, not the growing; a whole grow stopped leaves its marks set and the species finish frame by frame after. [assumed · coder 117-06 · 2026-11-20]
- A species deleted from Assets keeps drawing its last growth until the project is opened again. [assumed · planner 117 · 2026-11-20]

## In the game
- The game's model store is never emptied by an update or a restart, so species grown at start stay held the whole run and nothing regrows them. [observed · coder 117-18 · 2026-11-24]
- The game grows its species once, at start between the models update and the materials, not after each models update, since every grow regrows and reuploads every variation. [assumed · coder 117-18 · 2026-11-24] see 0515
- A shipped game reads its species from a cooked trees table beside the materials. [observed · coder 117-17 · 2026-11-24]
- A `.tree` that will not read leniently refuses the whole cook, naming the file, as a bad `.material` or `.parts` does. [assumed · coder 117-17 · 2026-11-24]
