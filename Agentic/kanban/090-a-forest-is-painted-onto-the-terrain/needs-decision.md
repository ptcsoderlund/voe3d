# Needs decision — how much model geometry the card holds, and whether that room grows

For bug 01 (`bugs/01-a-downloaded-fir-tree-does-not-import.md`).

## What blocks the tree
I ran the real file through the engine's own libraries:

1. `assets`: Blender writes `"texCoord": -1` on the three texture references of `fir_tree_01_trunk_a`,
   whose part has no UV map. The reader takes it as a broken index. That is the line the human saw.
2. `assets`: three of the six pictures are 16-bit PNGs. The PNG decoder refuses any depth but 8, and
   prints nothing.
3. With both patched in a scratch copy, the read succeeds. `voe_3d_model_levels_make` makes all six
   LODs in 63 s, using 2.4 GB of RAM, in the debug build.
4. **The tree then cannot fit on the card.** The model store's room is fixed when the device is created:
   `VOE_3D_MODELS_VERTICES` 3 M and `_INDICES` 9 M for every model together (`3d/include/3d/models.h`),
   added into `VOE_EDITOR_CAPACITIES` and `VOE_GAME_CAPACITIES`. `render`'s header says "nothing here
   grows". A vertex is 40 bytes. The tree's LOD 1 alone has 5,428,923 vertices and 12,530,457 indices.
   Made LODs are uploaded as compacted copies, so with its LODs the tree needs about 10.7 M vertices
   (430 MB) and 24.7 M indices (99 MB).

Points 1 and 2 and the 0443 wording are this feature's to plan. Point 4 is not. It decides how every
program's geometry memory on the card is budgeted, and it reverses a contract `render` states for all
its callers.

## The question
Bug 01 asks that a model import "however big, as long as the machine can hold it". Should the
geometry pools on the card grow to hold such a model, or stay a fixed room the engine reports as its
limit (0443)?

## Options
1. **Static geometry pools grow.** `vertices` and `indices` in `voe_render_capacities` become starting
   sizes. When `voe_render_geometry_create` finds no room, it makes a larger pool, copies the old one
   across and rebinds it. Create is already a startup call that waits for the card to go idle, so it
   can do this. The only limit left is the card's memory, and running out is reported in 0443's terms.
   Small projects pay nothing extra. Costs: one `render` card that reverses "nothing grows", and a
   short peak of the old pool plus the new one while it grows.
2. **A larger fixed room.** For example 16 M vertices and 48 M indices. That reserves 640 MB plus
   192 MB on the card in every editor and game, even for a scene with one cube, which hurts on the
   sponsor's laptops. It is still a limit, only a higher one.
3. **Keep 3 M / 9 M and say so.** Importing the tree reports, per 0443: "`fir_tree_01_4k.glb` has
   5,428,923 vertices, about 10.7 M with its LODs; the engine holds 3,145,728 for all models
   together; reduce it in Blender with Decimate". This is the simplest option, but the tree does not
   import, so bug 01's Expected is not met.

Any of the three can be paired with a change local to `3d`. Made LODs would index LOD 1's vertices
instead of uploading compacted copies. That needs only the indices (99 MB for this tree) and no more
vertices, and roughly halves what a model needs.

## Recommendation
Option 1, with the shared vertices of LOD 1 added. It is what bug 01 asks for and what 0280 and 0376
point to ("no refusing", downloaded models). Creating geometry already happens while the card is idle,
so growing the pools is a small change in one place. And a project only pays for the models it actually
loads. Once this is decided, the planner cuts the cards: Blender's `texCoord -1`, 16-bit PNGs, the
pools, and the import, load and LOD messages brought to 0443.
