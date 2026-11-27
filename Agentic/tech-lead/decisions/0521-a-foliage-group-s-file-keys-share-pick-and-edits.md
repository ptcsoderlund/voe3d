# 0521 — A foliage group's file keys, share, pick and edits
date: 2026-11-27
by: planner

## Decision
For 117 under 0520.

1. The file. `[Foliage]` spells glb J of group G `glb<G>_<J>="path"`, G from 0 up to the first G with no
   `glb<G>_0`, J from 0 up to the first missing. Each glb's look keys are today's kind keys with `<G>_<J>`
   for `<K>` (`size`, `vary`, `turn`, `tilt`, `shadow`, `blend`, `blendheight`) plus `share<G>_<J>`, each
   written only off its default. `seed<G>` is the group's. `grow<L>_<G>` and `c<i>=<G> x z turn scale lean`
   keep their shape with G a group. A file spelled with `kind<K>` keys reads, leniently and strictly, each
   kind as group K holding one glb, its look keys as that glb's and `seed<K>` as the group's; both
   spellings in one file is malformed. The writer writes only groups.
2. A share is a whole number 0 to 10, default 1. A group whose shares are all 0 picks as if all were 1.
3. The pick is assets': `voe_assets_landscape_glb_pick`, the hash 3d's `voe_3d_tree_pick` used, of the
   group's seed and the copy's painted x and z in mm, taken modulo the shares' total and walked over the
   glbs in order. One glb picks 0; equal shares pick as `% count` did. Nothing of it is stored: a copy's
   glb follows the group's glbs and shares live, painted and grown alike. A copy keeps the turn, scale and
   lean drawn when it was painted whatever glb it picks later, as a painted copy keeps them today when a
   spread changes; base size, shadows and blend are the picked glb's, at placement and draw.
4. At most 16 groups and 8 glbs a group. A path is in a group once: a drop of one the group holds, or a
   replace to one, is refused. The same glb may be in several groups.
5. Removing a group or a glb is one landscape step holding two whole landscapes, as 115's disk take is,
   with folds and picked groups following. A group's only glb has no ×: the group's × removes it. Adding
   a group or a glb and replacing a glb's path stay as a kind's drop is today: unsaved, no undo step. A
   share is part of the glb's look and changes as a spread does.
6. The Foliage section: one folding box per group, titled by its first glb's file name and, when it
   holds more, how many more; inside it a folding box per glb titled by its file name with its settings.
   A drop onto a group's title adds a glb, onto a glb's title replaces it, onto the add row makes a group.
   Folds are named `group <G> <path>` and `glb <G> <J> <path>`; a person's old `kind` folds read as open.
   Picking is per group.
7. The game's cooked landscape carries `voe_assets_landscape_group` values, each look written by field
   name.

## Reasoning
0520 settles what a group is and leaves its spelling, the share's range, the pick and the edit steps to
117. Keys indexed `<G>_<J>` read beside the existing `grow<L>_<K>` and keep one line per value, so a
person editing a share by hand (117's step 11) finds it as they find a size. The old spelling read as
one-glb groups opens every saved project unchanged (0520). Whole-number shares to 10 are enough for "this
one three times as often" and keep the file plain. Keeping the pick live rather than stored makes a share
edit and a glb removal reach copies already painted, which 117's steps 3 and 7 ask for, and costs one hash
a copy a draw, as the variation pick did. A glb removal as a whole-landscape step reuses what 115 built;
making adds undoable too would be a new behaviour the sponsor did not ask for. A path twice in one group
would only double its share by another name.
- A share as a percentage summing to 100: every edit would move the others.
- Storing each copy's glb: a share edit would reach only later strokes.
- A group title of its own name: a new field the file and panel would need; the first glb's name is
  what a one-glb group shows today.

## Replaces
Nothing.
