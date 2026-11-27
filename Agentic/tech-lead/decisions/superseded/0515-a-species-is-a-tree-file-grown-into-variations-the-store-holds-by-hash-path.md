# 0515 — A species is a `.tree` file, grown into variations the store holds at `<path>#<v>`
date: 2026-11-20
by: planner

## Decision
1. **The file.** A species is `<name>.tree`: sectioned text with one `[Tree]` section. Its keys, with
   range and default:

   | Key | Meaning | Range | Default |
   |---|---|---|---|
   | `bark`, `leaf` | quoted `.material` paths | — | empty |
   | `height` | metres | 1–60 | 12 |
   | `trunk` | base diameter, metres | 0.05–3 | 0.4 |
   | `crown` | share of the height where branches start | 0–0.9 | 0.3 |
   | `branching` | branches per metre of parent | 0.5–20 | 4 |
   | `reach` | a child's length as a share of its parent's | 0.05–1 | 0.35 |
   | `angle` | degrees from the parent | 5–120 | 60 |
   | `droop` | — | −1–1 | 0.2 |
   | `crooked` | — | 0–1 | 0.2 |
   | `leaf_size` | metres | 0.02–2 | 0.3 |
   | `leaf_density` | cards per twig | 0–64 | 12 |
   | `variations` | — | 1–16 | 4 |

   The lenient reader clamps and defaults, as a material's does. The strict reader refuses an unknown key
   or section and a value out of range (0509, 0498). Docs come from its key table (0511, 0512).
2. **Growth.** A tree grows deterministically from its values and its variation index, using xorshift.
   It has three orders: trunk, branches above `crown` × height, then twigs; leaf cards sit on the twigs.
   A child starts inside its parent's axis with a collar ring widened to the parent's surface, so it
   reads as joined.
   The grower works in two steps: a skeleton (segments with ends, radii, parent and order), then a mesh
   per level (0514). The skeleton is not stored; the steps are where wind and chopping can take it later.
   The output is an in-memory `voe_assets_model` with material 0 for bark and 1 for leaf, prepared with
   its own levels.
3. **The store.** Variation v is the entry `<path>#<v>`, counting from 0. A species entry at `<path>`
   holds the count and the variations; a foliage kind names it.
   Grown entries wear their bark and leaf paths themselves: no parts table touches them, no watch reads
   them as files, and renaming the species renames its variations.
4. **The pick.** A copy draws variation hash(kind index, x mm, z mm) mod count (0513). The x and z are the
   integers the copy stores, so a pick survives saving and reopening.
5. **Growing.** Play and a shipped game grow every species of the cooked trees table at start, under the
   splash ("Growing trees"), each variation a task on one burst (0485, 0504). The editor does the same
   under its own splash.
   After a change, the editor regrows one species at a time on a burst polled between frames and uploads
   one variation per frame (0386).
6. **The species view.** Opening a `.tree` sets the level aside as an open prefab does (0283), using the
   same field, so Back and the refusal of Play and Ship apply unchanged. The world holds a sun and one
   model row wearing the stepped variation.
   Save writes no world while a species is open: each edit writes the species file at once, as a
   material edit does.
7. **Leaves** wear the leaf material as authored. Whether they are cut out and double-sided is that
   material's Cutout and Both sides.

## Reasoning
These are the smallest pieces that keep "nothing but the species file on disk" (0510).
- An entry per variation means the store, its levels, its dissolve and its foliage draw work unchanged.
  The `#` cannot occur in an `Assets/` file name the editor makes, so it never meets a real file.
- Reusing the open prefab's set-aside gives the species view Back, the Play refusal and the top bar for free.
- The species file names materials by path, so the sponsor's own needle material decides cut-out and
  sides, as it does on any model.
- `crown` is added because without it branches grow down to the ground, and no spruce or birch does.
- Sixteen variations fit the store's 128 entries beside the hill's other models.

## Replaces
Nothing. Carries out 0510 and 0513 for 117.
